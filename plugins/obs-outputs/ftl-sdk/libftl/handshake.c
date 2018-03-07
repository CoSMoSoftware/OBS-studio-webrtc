/**
 * activate.c - Activates an FTL stream
 *
 * Copyright (c) 2015 Michael Casadevall
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

#define __FTL_INTERNAL
#include "ftl.h"
#include "ftl_private.h"
#include <stdarg.h>

OS_THREAD_ROUTINE  connection_status_thread(void *data);
OS_THREAD_ROUTINE  control_keepalive_thread(void *data);
static ftl_response_code_t _ftl_get_response(ftl_stream_configuration_private_t *ftl, char *response_buf, int response_len);

static ftl_response_code_t _ftl_send_command(ftl_stream_configuration_private_t *ftl_cfg, BOOL need_response, char *response_buf, int response_len, const char *cmd_fmt, ...);
ftl_status_t _log_response(ftl_stream_configuration_private_t *ftl, int response_code);

ftl_status_t _init_control_connection(ftl_stream_configuration_private_t *ftl) {
  int err = 0;
  SOCKET sock = 0;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  ftl_status_t retval = FTL_SUCCESS;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  char ingest_ip[IPVX_ADDR_ASCII_LEN];

  struct addrinfo* resolved_names = 0;
  struct addrinfo* p = 0;

  int ingest_port = INGEST_PORT;
  char ingest_port_str[10];

  if (ftl_get_state(ftl, FTL_CONNECTED)) {
    return FTL_ALREADY_CONNECTED;
  }

  snprintf(ingest_port_str, 10, "%d", ingest_port);

  if ((retval = _set_ingest_hostname(ftl)) != FTL_SUCCESS) {
    return retval;
  }
  
  err = getaddrinfo(ftl->ingest_hostname, ingest_port_str, &hints, &resolved_names);
  if (err != 0) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "getaddrinfo failed to look up ingest address %s.", ftl->ingest_hostname);
    FTL_LOG(ftl, FTL_LOG_ERROR, "gai error was: %s", gai_strerror(err));
    return FTL_DNS_FAILURE;
  }

  /* Open a socket to the control port */
  for (p = resolved_names; p != NULL; p = p->ai_next) {
    sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sock == -1) {
      /* try the next candidate */
      FTL_LOG(ftl, FTL_LOG_DEBUG, "failed to create socket. error: %s", get_socket_error());
      continue;
    }

  if (p->ai_family == AF_INET) {
    struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)p->ai_addr;
    inet_ntop(p->ai_family, &ipv4_addr->sin_addr, ingest_ip, sizeof(ingest_ip));
  }
  else if (p->ai_family == AF_INET6) {
    struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6 *)p->ai_addr;
    inet_ntop(p->ai_family, &ipv6_addr->sin6_addr, ingest_ip, sizeof(ingest_ip));
  }
  else {
  continue;
  }

  FTL_LOG(ftl, FTL_LOG_DEBUG, "Got IP: %s\n", ingest_ip);
  ftl->ingest_ip = _strdup(ingest_ip);
  ftl->socket_family = p->ai_family;

    /* Go for broke */
    if (connect(sock, p->ai_addr, (int)p->ai_addrlen) == -1) {
      FTL_LOG(ftl, FTL_LOG_DEBUG, "failed to connect on candidate, error: %s", get_socket_error());
      close_socket(sock);
      sock = 0;
      continue;
    }

    /* If we got here, we successfully connected */
    if (set_socket_enable_keepalive(sock) != 0) {
      FTL_LOG(ftl, FTL_LOG_DEBUG, "failed to enable keep alives.  error: %s", get_socket_error());
    }

    if (set_socket_recv_timeout(sock, SOCKET_RECV_TIMEOUT_MS) != 0) {
      FTL_LOG(ftl, FTL_LOG_DEBUG, "failed to set recv timeout.  error: %s", get_socket_error());
    }

    if (set_socket_send_timeout(sock, SOCKET_SEND_TIMEOUT_MS) != 0) {
      FTL_LOG(ftl, FTL_LOG_DEBUG, "failed to set send timeout.  error: %s", get_socket_error());
    }

    break;
  }

  /* Free the resolved name struct */
  freeaddrinfo(resolved_names);

  /* Check to see if we actually connected */
  if (sock <= 0) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "failed to connect to ingest. Last error was: %s",
      get_socket_error());
    return FTL_CONNECT_ERROR;
  }

  ftl->ingest_socket = sock;
  
  return FTL_SUCCESS;
}

ftl_status_t _ingest_connect(ftl_stream_configuration_private_t *ftl) {
  ftl_response_code_t response_code = FTL_INGEST_RESP_UNKNOWN;
  char response[MAX_INGEST_COMMAND_LEN];

  if (ftl_get_state(ftl, FTL_CONNECTED)) {
    return FTL_ALREADY_CONNECTED;
  }

  if (ftl->ingest_socket <= 0) {
    return FTL_SOCKET_NOT_CONNECTED;
  }

  do {

    if (!ftl_get_hmac(ftl->ingest_socket, ftl->key, ftl->hmacBuffer)) {
      FTL_LOG(ftl, FTL_LOG_ERROR, "could not get a signed HMAC!");
      response_code = FTL_INGEST_NO_RESPONSE;
      break;
    }

    if ((response_code = _ftl_send_command(ftl, TRUE, response, sizeof(response), "CONNECT %d $%s", ftl->channel_id, ftl->hmacBuffer)) != FTL_INGEST_RESP_OK) {
      break;
    }

    /* We always send our version component first */
    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "ProtocolVersion: %d.%d", FTL_VERSION_MAJOR, FTL_VERSION_MINOR)) != FTL_INGEST_RESP_OK) {
      break;
    }

    /* Cool. Now ingest wants our stream meta-data, which we send as key-value pairs, followed by a "." */
    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "VendorName: %s", ftl->vendor_name)) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "VendorVersion: %s", ftl->vendor_version)) != FTL_INGEST_RESP_OK) {
      break;
    }

    ftl_video_component_t *video = &ftl->video;
    /* We're sending video */
    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "Video: true")) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "VideoCodec: %s", ftl_video_codec_to_string(video->codec))) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "VideoHeight: %d", video->height)) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "VideoWidth: %d", video->width)) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "VideoPayloadType: %d", video->media_component.payload_type)) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "VideoIngestSSRC: %d", video->media_component.ssrc)) != FTL_INGEST_RESP_OK) {
      break;
    }

    ftl_audio_component_t *audio = &ftl->audio;

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "Audio: true")) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "AudioCodec: %s", ftl_audio_codec_to_string(audio->codec))) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "AudioPayloadType: %d", audio->media_component.payload_type)) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "AudioIngestSSRC: %d", audio->media_component.ssrc)) != FTL_INGEST_RESP_OK) {
      break;
    }

    if ((response_code = _ftl_send_command(ftl, TRUE, response, sizeof(response), ".")) != FTL_INGEST_RESP_OK) {
      break;
    }

    /*see if there is a port specified otherwise use default*/
    int port = ftl_read_media_port(response);

    ftl->media.assigned_port = port;

    ftl_set_state(ftl, FTL_CONNECTED);

    if (os_semaphore_create(&ftl->connection_thread_shutdown, "/ConnectionThreadShutdown", O_CREAT, 0, FALSE) < 0) {
        response_code = FTL_MALLOC_FAILURE;
        break;
    }

    if (os_semaphore_create(&ftl->keepalive_thread_shutdown, "/KeepAliveThreadShutdown", O_CREAT, 0, FALSE) < 0) {
        response_code = FTL_MALLOC_FAILURE;
        break;
    }

    ftl_set_state(ftl, FTL_CXN_STATUS_THRD);
    if ((os_create_thread(&ftl->connection_thread, NULL, connection_status_thread, ftl)) != 0) {
      ftl_clear_state(ftl, FTL_CXN_STATUS_THRD);
      response_code = FTL_MALLOC_FAILURE;
      break;
    }

    ftl_set_state(ftl, FTL_KEEPALIVE_THRD);
    if ((os_create_thread(&ftl->keepalive_thread, NULL, control_keepalive_thread, ftl)) != 0) {
      ftl_clear_state(ftl, FTL_KEEPALIVE_THRD);\
      response_code = FTL_MALLOC_FAILURE;
      break;
    }

    FTL_LOG(ftl, FTL_LOG_INFO, "Successfully connected to ingest.  Media will be sent to port %d\n", ftl->media.assigned_port);
  
    return FTL_SUCCESS;
  } while (0);

  _ingest_disconnect(ftl);

  return _log_response(ftl, response_code);;
}

ftl_status_t _ingest_disconnect(ftl_stream_configuration_private_t *ftl) {

    ftl_response_code_t response_code = FTL_INGEST_RESP_UNKNOWN;
    char response[MAX_INGEST_COMMAND_LEN];

    if (ftl_get_state(ftl, FTL_KEEPALIVE_THRD)) {
        ftl_clear_state(ftl, FTL_KEEPALIVE_THRD);
        os_semaphore_post(&ftl->keepalive_thread_shutdown);
        os_wait_thread(ftl->keepalive_thread);
        os_destroy_thread(ftl->keepalive_thread);
        os_semaphore_delete(&ftl->keepalive_thread_shutdown);
    }

    if (ftl_get_state(ftl, FTL_CXN_STATUS_THRD)) {
        ftl_clear_state(ftl, FTL_CXN_STATUS_THRD);
        os_semaphore_post(&ftl->connection_thread_shutdown);
        os_wait_thread(ftl->connection_thread);
        os_destroy_thread(ftl->connection_thread);
        os_semaphore_delete(&ftl->connection_thread_shutdown);
    }

    if (ftl_get_state(ftl, FTL_BITRATE_THRD))
    {
        ftl_clear_state(ftl, FTL_BITRATE_THRD);
        os_semaphore_post(&ftl->bitrate_thread_shutdown);
        os_wait_thread(ftl->bitrate_monitor_thread);
        os_destroy_thread(ftl->bitrate_monitor_thread);
        os_semaphore_delete(&ftl->bitrate_thread_shutdown);
    }

    if (ftl_get_state(ftl, FTL_CONNECTED)) {

        ftl_clear_state(ftl, FTL_CONNECTED);

        FTL_LOG(ftl, FTL_LOG_INFO, "light-saber disconnect\n");
        if ((response_code = _ftl_send_command(ftl, FALSE, response, sizeof(response), "DISCONNECT", ftl->channel_id)) != FTL_INGEST_RESP_OK) {
            FTL_LOG(ftl, FTL_LOG_ERROR, "Ingest Disconnect failed with %d (%s)\n", response_code, response);
        }
    }

    if (ftl->ingest_socket > 0) {
        close_socket(ftl->ingest_socket);
        ftl->ingest_socket = 0;
    }

    return FTL_SUCCESS;
}

static ftl_response_code_t _ftl_get_response(ftl_stream_configuration_private_t *ftl, char *response_buf, int response_len){
  int len;
  memset(response_buf, 0, response_len);
  len = recv_all(ftl->ingest_socket, response_buf, response_len, '\n');

  if (len <= 0) {

#ifdef _WIN32
    int error = WSAGetLastError();
    if (error == WSAETIMEDOUT)
    {
      return FTL_INGEST_RESP_INTERNAL_SOCKET_TIMEOUT;
    }
    else
    {
      return FTL_INGEST_RESP_INTERNAL_SOCKET_CLOSED;
    }
#else
    if (len == 0)
    {
      return FTL_INGEST_RESP_INTERNAL_SOCKET_CLOSED;
    }
    else
    {
      return FTL_INGEST_RESP_INTERNAL_SOCKET_TIMEOUT;
    }
#endif
  }

  return ftl_read_response_code(response_buf);
}

static ftl_response_code_t _ftl_send_command(ftl_stream_configuration_private_t *ftl, BOOL need_response, char *response_buf, int response_len, const char *cmd_fmt, ...) {
  int resp_code = FTL_INGEST_RESP_OK;
  va_list valist;
  char *buf = NULL;
  int len;
  int buflen = MAX_INGEST_COMMAND_LEN * sizeof(char);
  char *format = NULL;

  do {
    if ((buf = (char*)malloc(buflen)) == NULL) {
      resp_code = FTL_INGEST_RESP_INTERNAL_MEMORY_ERROR;
      break;
    }

    if ((format = (char*)malloc(strlen(cmd_fmt) + 5)) == NULL) {
      resp_code = FTL_INGEST_RESP_INTERNAL_MEMORY_ERROR;
      break;
    }

    sprintf_s(format, strlen(cmd_fmt) + 5, "%s\r\n\r\n", cmd_fmt);

    va_start(valist, cmd_fmt);

    memset(buf, 0, buflen);

    len = vsnprintf(buf, buflen, format, valist);

    va_end(valist);

    if (len < 0 || len >= buflen) {
      resp_code = FTL_INGEST_RESP_INTERNAL_COMMAND_ERROR;
      break;
    }

    send(ftl->ingest_socket, buf, len, 0);

    if (need_response) {
      resp_code = _ftl_get_response(ftl, response_buf, response_len);
    }
  } while (0);

  if(buf != NULL){
    free(buf);
  }

  if(format != NULL){
      free(format);
  }

  return resp_code;
}

OS_THREAD_ROUTINE control_keepalive_thread(void *data)
{
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)data;
  ftl_response_code_t response_code;
  struct timeval last_send_time, now;
  int64_t ms_since_send = 0;

  gettimeofday(&last_send_time, NULL);

  while (ftl_get_state(ftl, FTL_KEEPALIVE_THRD)) {
    os_semaphore_pend(&ftl->keepalive_thread_shutdown, KEEPALIVE_FREQUENCY_MS);

    if (!ftl_get_state(ftl, FTL_KEEPALIVE_THRD))
    {
      break;
    }

    // Check how long it has been since we sent a ping last.
    gettimeofday(&now, NULL);
    ms_since_send = timeval_subtract_to_ms(&now, &last_send_time);
    if (ms_since_send > KEEPALIVE_FREQUENCY_MS + KEEPALIVE_SEND_WARN_TOLERANCE_MS)
    {
       FTL_LOG(ftl, FTL_LOG_INFO, "Warning, ping time tolerance warning. Time since last ping %d ms", ms_since_send);
    }
    gettimeofday(&last_send_time, NULL);

    // Send the ping to ingest now.
    if ((response_code = _ftl_send_command(ftl, FALSE, NULL, 0, "PING %d", ftl->channel_id)) != FTL_INGEST_RESP_OK) {
      FTL_LOG(ftl, FTL_LOG_ERROR, "Ingest ping failed with %d\n", response_code);
    }
  }

  FTL_LOG(ftl, FTL_LOG_INFO, "Exited control_keepalive_thread\n");

  return 0;
}

OS_THREAD_ROUTINE connection_status_thread(void *data)
{
    ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)data;
    char buf[1024];
    ftl_status_msg_t status;
    struct timeval last_ping, now;
    int64_t ms_since_ping = 0;

    // We ping every 5 seconds, but don't timeout the connection until 30 seconds has passed
    // without hearing anything back from the ingest. This time is high, but some some poor networks
    // this can happen.
    int keepalive_is_late = 6 * KEEPALIVE_FREQUENCY_MS; 

    gettimeofday(&last_ping, NULL);

    // Loop while the connection status thread should be alive.
    while (ftl_get_state(ftl, FTL_CXN_STATUS_THRD)) {

        // Wait on the shutdown event for at most STATUS_THREAD_SLEEP_TIME_MS
        os_semaphore_pend(&ftl->connection_thread_shutdown, STATUS_THREAD_SLEEP_TIME_MS);
        if (!ftl_get_state(ftl, FTL_CXN_STATUS_THRD))
        {
            break;
        }

        ftl_status_t error_code = FTL_SUCCESS;

        // Check if there is any data for us to consume.
        unsigned long bytesAvailable = 0;
        int ret = get_socket_bytes_available(ftl->ingest_socket, &bytesAvailable);
        if (ret < 0)
        {
            FTL_LOG(ftl, FTL_LOG_ERROR, "Failed to call get_socket_bytes_available, %s", get_socket_error());
            error_code = FTL_UNKNOWN_ERROR_CODE;
        }
        else
        {
            // If we have data waiting, consume it now.
            if (bytesAvailable > 0)
            {
                int resp_code = _ftl_get_response(ftl, buf, sizeof(buf));

                // If we got a ping response, mark the time and loop again.
                if (resp_code  == FTL_INGEST_RESP_PING) {
                    gettimeofday(&last_ping, NULL);
                    continue;
                }

                // If it's anything else, it's an error.
                error_code = _log_response(ftl, resp_code);
            }
        }

        // If we don't have an error, check if the ping has timed out.
        if (error_code == FTL_SUCCESS)
        {
            // Get the current time and figure out the time since the last ping was recieved.
            gettimeofday(&now, NULL);
            ms_since_ping = timeval_subtract_to_ms(&now, &last_ping);
            if (ms_since_ping < keepalive_is_late) {
                continue;
            }

            // Otherwise, we havn't gotten the ping in too long.
            FTL_LOG(ftl, FTL_LOG_ERROR, "Ingest ping timeout, we haven't gotten a ping in %d ms.", ms_since_ping);
            error_code = FTL_NO_PING_RESPONSE;
        }

        // At this point something is wrong, and we are going to shutdown the connection. Do one more check that we
        // should still be running.
        if (!ftl_get_state(ftl, FTL_CXN_STATUS_THRD))
        {
            break;
        }
        FTL_LOG(ftl, FTL_LOG_ERROR, "Ingest connection has dropped: error code %d\n", error_code);

        // Clear the state that this thread is running. If we don't do this we will dead lock
        // in the internal_ingest_disconnect.
        ftl_clear_state(ftl, FTL_CXN_STATUS_THRD);

        // Shutdown the ingest connection.
        if (os_trylock_mutex(&ftl->disconnect_mutex)) {
            internal_ingest_disconnect(ftl);
            os_unlock_mutex(&ftl->disconnect_mutex);
        }

        // Fire an event indicating we shutdown.
        status.type = FTL_STATUS_EVENT;
        if (error_code == FTL_NO_MEDIA_TIMEOUT) {
            status.msg.event.reason = FTL_STATUS_EVENT_REASON_NO_MEDIA;
        }
        else {
            status.msg.event.reason = FTL_STATUS_EVENT_REASON_UNKNOWN;
        }
        status.msg.event.type = FTL_STATUS_EVENT_TYPE_DISCONNECTED;
        status.msg.event.error_code = error_code;
        enqueue_status_msg(ftl, &status);

        // Exit the loop.
        break;		
    }

    FTL_LOG(ftl, FTL_LOG_INFO, "Exited connection_status_thread");
    return 0;
}

ftl_status_t _log_response(ftl_stream_configuration_private_t *ftl, int response_code){

  switch (response_code) {
    case FTL_INGEST_RESP_OK:
      FTL_LOG(ftl, FTL_LOG_DEBUG, "ingest accepted our paramteres");
    return FTL_SUCCESS;
    case FTL_INGEST_NO_RESPONSE:
      FTL_LOG(ftl, FTL_LOG_ERROR, "ingest did not respond to request");
      return FTL_INGEST_NO_RESPONSE;
    case FTL_INGEST_RESP_PING:
      return FTL_SUCCESS;
    case FTL_INGEST_RESP_BAD_REQUEST:
      FTL_LOG(ftl, FTL_LOG_ERROR, "ingest responded bad request");
      return FTL_BAD_REQUEST;
    case FTL_INGEST_RESP_UNAUTHORIZED:
      FTL_LOG(ftl, FTL_LOG_ERROR, "channel is not authorized for FTL");
      return FTL_UNAUTHORIZED;
    case FTL_INGEST_RESP_OLD_VERSION:
      FTL_LOG(ftl, FTL_LOG_ERROR, "This version of the FTLSDK is depricated");
      return FTL_OLD_VERSION;
    case FTL_INGEST_RESP_AUDIO_SSRC_COLLISION:
      FTL_LOG(ftl, FTL_LOG_ERROR, "audio SSRC collision from this IP address. Please change your audio SSRC to an unused value");
      return FTL_AUDIO_SSRC_COLLISION;
    case FTL_INGEST_RESP_VIDEO_SSRC_COLLISION:
      FTL_LOG(ftl, FTL_LOG_ERROR, "video SSRC collision from this IP address. Please change your audio SSRC to an unused value");
      return FTL_VIDEO_SSRC_COLLISION;
    case FTL_INGEST_RESP_INVALID_STREAM_KEY:
      FTL_LOG(ftl, FTL_LOG_ERROR, "The stream key or channel id is incorrect");
      return FTL_BAD_OR_INVALID_STREAM_KEY;
    case FTL_INGEST_RESP_CHANNEL_IN_USE:
      FTL_LOG(ftl, FTL_LOG_ERROR, "the channel id is already actively streaming");
      return FTL_CHANNEL_IN_USE;
    case FTL_INGEST_RESP_REGION_UNSUPPORTED:
      FTL_LOG(ftl, FTL_LOG_ERROR, "the region is not authorized to stream");
      return FTL_REGION_UNSUPPORTED;
    case FTL_INGEST_RESP_NO_MEDIA_TIMEOUT:
      FTL_LOG(ftl, FTL_LOG_ERROR, "The server did not receive media (audio or video) for an extended period of time");
      return FTL_NO_MEDIA_TIMEOUT;
    case FTL_INGEST_RESP_INTERNAL_SERVER_ERROR:
      FTL_LOG(ftl, FTL_LOG_ERROR, "parameters accepted, but ingest couldn't start FTL. Please contact support!");
      return FTL_INTERNAL_ERROR;
    case FTL_INGEST_RESP_GAME_BLOCKED:
      FTL_LOG(ftl, FTL_LOG_ERROR, "The current game set by this profile can't be streamed.");
      return FTL_GAME_BLOCKED;
    case FTL_INGEST_RESP_INTERNAL_MEMORY_ERROR:
      FTL_LOG(ftl, FTL_LOG_ERROR, "Server memory error");
      return FTL_INTERNAL_ERROR;
    case FTL_INGEST_RESP_INTERNAL_COMMAND_ERROR:
      FTL_LOG(ftl, FTL_LOG_ERROR, "Server command error");
      return FTL_INTERNAL_ERROR;
    case FTL_INGEST_RESP_INTERNAL_SOCKET_CLOSED:
      FTL_LOG(ftl, FTL_LOG_ERROR, "Ingest socket closed.");
      return FTL_INGEST_SOCKET_CLOSED;
    case FTL_INGEST_RESP_INTERNAL_SOCKET_TIMEOUT:
      FTL_LOG(ftl, FTL_LOG_ERROR, "Ingest socket timeout.");
      return FTL_INGEST_SOCKET_TIMEOUT;
    case FTL_INGEST_RESP_UNKNOWN:
        FTL_LOG(ftl, FTL_LOG_ERROR, "Ingest unknown response.");
        return FTL_INTERNAL_ERROR;
  }    

  // TODO revert back
  return 100 + response_code;
}
