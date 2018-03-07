
#define __FTL_INTERNAL
#include "ftl.h"
#include "ftl_private.h"
#ifndef DISABLE_AUTO_INGEST
#include <curl/curl.h>
#endif

static BOOL _get_chan_id_and_key(const char *stream_key, uint32_t *chan_id, char *key);
static int _lookup_ingest_ip(const char *ingest_location, char *ingest_ip);

char error_message[1000];
FTL_API const int FTL_VERSION_MAJOR = 0;
FTL_API const int FTL_VERSION_MINOR = 9;
FTL_API const int FTL_VERSION_MAINTENANCE = 13;

// Initializes all sublibraries used by FTL
FTL_API ftl_status_t ftl_init() {
  struct timeval now;

  init_sockets();
  os_init();
#ifndef DISABLE_AUTO_INGEST
  curl_global_init(CURL_GLOBAL_ALL);
#endif

  gettimeofday(&now, NULL);
  srand((unsigned int)now.tv_sec);
  return FTL_SUCCESS;
}

FTL_API ftl_status_t ftl_ingest_create(ftl_handle_t *ftl_handle, ftl_ingest_params_t *params){
  ftl_status_t ret_status = FTL_SUCCESS;
  ftl_stream_configuration_private_t *ftl = NULL;

  do {
    if ((ftl = (ftl_stream_configuration_private_t *)malloc(sizeof(ftl_stream_configuration_private_t))) == NULL) {
      // Note it is important that we return here otherwise the call to 
      // internal_ftl_ingest_destroy will fail!
      return FTL_MALLOC_FAILURE;
    }

    memset(ftl, 0, sizeof(ftl_stream_configuration_private_t));

    // First create any components that the system relies on.
    os_init_mutex(&ftl->state_mutex);
    os_init_mutex(&ftl->disconnect_mutex);
    os_init_mutex(&ftl->status_q.mutex);

    if (os_semaphore_create(&ftl->status_q.sem, "/StatusQueue", O_CREAT, 0, FALSE) < 0) {
        ret_status = FTL_MALLOC_FAILURE;
        break;
    }

    // Capture the incoming key.
    ftl->key = NULL;
    if ((ftl->key = (char*)malloc(sizeof(char)*MAX_KEY_LEN)) == NULL) {
      ret_status = FTL_MALLOC_FAILURE;
      break;
    }
    if (_get_chan_id_and_key(params->stream_key, &ftl->channel_id, ftl->key) == FALSE) {
      ret_status = FTL_BAD_OR_INVALID_STREAM_KEY;
      break;
    }

    ftl->audio.codec = params->audio_codec;
    ftl->video.codec = params->video_codec;

    ftl->audio.media_component.payload_type = AUDIO_PTYPE;
    ftl->video.media_component.payload_type = VIDEO_PTYPE;

    //TODO: this should be randomly generated, there is a potential for ssrc collisions with this
    ftl->audio.media_component.ssrc = ftl->channel_id;
    ftl->video.media_component.ssrc = ftl->channel_id + 1;

    ftl->video.fps_num = params->fps_num;
    ftl->video.fps_den = params->fps_den;
    ftl->video.dts_usec = 0;
    ftl->audio.dts_usec = 0;
    ftl->video.dts_error = 0;

    strncpy_s(ftl->vendor_name, sizeof(ftl->vendor_name) / sizeof(ftl->vendor_name[0]), params->vendor_name, sizeof(ftl->vendor_name) / sizeof(ftl->vendor_name[0]) - 1);
    strncpy_s(ftl->vendor_version, sizeof(ftl->vendor_version) / sizeof(ftl->vendor_version[0]), params->vendor_version, sizeof(ftl->vendor_version) / sizeof(ftl->vendor_version[0]) - 1);

    /*this is legacy, this isnt used anymore*/
    ftl->video.width = 1280;
    ftl->video.height = 720;

    ftl->video.media_component.peak_kbps = params->peak_kbps;
    ftl->param_ingest_hostname = _strdup(params->ingest_hostname);

    ftl->status_q.count = 0;
    ftl->status_q.head = NULL;

    ftl_set_state(ftl, FTL_STATUS_QUEUE);

    ftl_handle->priv = ftl;
    return ret_status;
  } while (0);

  internal_ftl_ingest_destroy(ftl);

  return ret_status;  
}

FTL_API ftl_status_t ftl_ingest_connect(ftl_handle_t *ftl_handle){
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;
  ftl_status_t status = FTL_SUCCESS;

  do {
    if ((status = _init_control_connection(ftl)) != FTL_SUCCESS) {
      break;
    }

    if ((status = _ingest_connect(ftl)) != FTL_SUCCESS) {
      break;
    }

    if ((status = media_init(ftl)) != FTL_SUCCESS) {
      break;
    }

    return status;
  } while (0);

  if (os_trylock_mutex(&ftl->disconnect_mutex)) {
    internal_ingest_disconnect(ftl);
    os_unlock_mutex(&ftl->disconnect_mutex);
  }
  
  return status;
}

FTL_API ftl_status_t ftl_ingest_get_status(ftl_handle_t *ftl_handle, ftl_status_msg_t *msg, int ms_timeout) {
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;

  if (ftl == NULL) {
    return FTL_NOT_INITIALIZED;
  }

  return dequeue_status_msg(ftl, msg, ms_timeout);
}

FTL_API ftl_status_t ftl_ingest_update_params(ftl_handle_t *ftl_handle, ftl_ingest_params_t *params) {
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;
  ftl_status_t status = FTL_SUCCESS;

  ftl->video.media_component.peak_kbps = params->peak_kbps;

  if (params->ingest_hostname != NULL) {
    if (ftl->param_ingest_hostname != NULL) {
      free(ftl->param_ingest_hostname);
      ftl->param_ingest_hostname = NULL;
    }

    ftl->param_ingest_hostname = _strdup(params->ingest_hostname);
  }

  /* not going to update fps for the moment*/
  /*
  ftl->video.fps_num = params->fps_num;
  ftl->video.fps_den = params->fps_den;
  */

  return status;
}

FTL_API int ftl_ingest_speed_test(ftl_handle_t *ftl_handle, int speed_kbps, int duration_ms) {

  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;

  speed_test_t results;

  FTL_LOG(ftl, FTL_LOG_WARN, "%s() is depricated, please use ftl_ingest_speed_test_ex()\n", __FUNCTION__);

  if (media_speed_test(ftl, speed_kbps, duration_ms, &results) == FTL_SUCCESS) {
    return results.peak_kbps;
  }

  return -1;
}

FTL_API ftl_status_t ftl_ingest_speed_test_ex(ftl_handle_t *ftl_handle, int speed_kbps, int duration_ms, speed_test_t *results) {

  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;

  return media_speed_test(ftl, speed_kbps, duration_ms, results);
}

FTL_API int ftl_ingest_send_media_dts(ftl_handle_t *ftl_handle, ftl_media_type_t media_type, int64_t dts_usec, uint8_t *data, int32_t len, int end_of_frame) {

  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;
  int bytes_sent = 0;

  if (media_type == FTL_AUDIO_DATA) {
    bytes_sent = media_send_audio(ftl, dts_usec, data, len);
  }
  else if (media_type == FTL_VIDEO_DATA) {
    bytes_sent = media_send_video(ftl, dts_usec, data, len, end_of_frame);
  }
  else {
    return bytes_sent;
  }

  return bytes_sent;
}

FTL_API int ftl_ingest_send_media(ftl_handle_t *ftl_handle, ftl_media_type_t media_type, uint8_t *data, int32_t len, int end_of_frame) {

  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;
  int64_t dts_increment_usec, dts_usec = 0;

  if (media_type == FTL_AUDIO_DATA) {
    dts_usec = ftl->audio.dts_usec;
    dts_increment_usec = AUDIO_PACKET_DURATION_MS * 1000;
    ftl->audio.dts_usec += dts_increment_usec;
  }
  else if (media_type == FTL_VIDEO_DATA) {
    dts_usec = ftl->video.dts_usec;
    if (end_of_frame) {
      float dst_usec_f = (float)ftl->video.fps_den * 1000000.f / (float)ftl->video.fps_num + ftl->video.dts_error;
      dts_increment_usec = (int64_t)(dst_usec_f);
      ftl->video.dts_error = dst_usec_f - (float)dts_increment_usec;
      ftl->video.dts_usec += dts_increment_usec;
    }
  }

  return ftl_ingest_send_media_dts(ftl_handle, media_type, dts_usec, data, len, end_of_frame);
}

FTL_API ftl_status_t ftl_ingest_disconnect(ftl_handle_t *ftl_handle) {
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;
  ftl_status_t status_code = FTL_SUCCESS;

  os_lock_mutex(&ftl->disconnect_mutex);

  if (ftl_get_state(ftl, FTL_CONNECTED)) {

    status_code = internal_ingest_disconnect(ftl);

    ftl_status_msg_t status;
    status.type = FTL_STATUS_EVENT;
    status.msg.event.reason = FTL_STATUS_EVENT_REASON_API_REQUEST;
    status.msg.event.type = FTL_STATUS_EVENT_TYPE_DISCONNECTED;
    status.msg.event.error_code = FTL_USER_DISCONNECT;

    enqueue_status_msg(ftl, &status);
  }

  os_unlock_mutex(&ftl->disconnect_mutex);

  return status_code;
}

ftl_status_t internal_ingest_disconnect(ftl_stream_configuration_private_t *ftl) {

  ftl_status_t status_code;
    
  ftl_set_state(ftl, FTL_DISCONNECT_IN_PROGRESS);

  if ((status_code = media_destroy(ftl)) != FTL_SUCCESS) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "failed to clean up media channel with error %d\n", status_code);
  }

  if ((status_code = _ingest_disconnect(ftl)) != FTL_SUCCESS) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "Disconnect failed with error %d\n", status_code);
  }

  ftl_clear_state(ftl, FTL_DISCONNECT_IN_PROGRESS);

    
  return FTL_SUCCESS;
}

ftl_status_t internal_ftl_ingest_destroy(ftl_stream_configuration_private_t *ftl) {

  if (ftl != NULL) {

    ftl_clear_state(ftl, FTL_STATUS_QUEUE);
    //if a thread is waiting send a destroy event
    if (ftl->status_q.thread_waiting) {
      ftl_status_msg_t status;
      status.type = FTL_STATUS_EVENT;
      status.msg.event.reason = FTL_STATUS_EVENT_REASON_API_REQUEST;
      status.msg.event.type = FTL_STATUS_EVENT_TYPE_DESTROYED;
      status.msg.event.error_code = FTL_SUCCESS;
      enqueue_status_msg(ftl, &status);
    }

    //wait a few ms for the thread to pull that last message and exit
    
    int  wait_retries = 5;
    while (ftl->status_q.thread_waiting && wait_retries-- > 0) {
      sleep_ms(20);
    };

    if (ftl->status_q.thread_waiting) {
      fprintf(stderr, "Thread is still waiting in ftl_ingest_get_status()\n");
    }

    os_lock_mutex(&ftl->status_q.mutex);

    status_queue_elmt_t *elmt;
    while (ftl->status_q.head != NULL) {
      elmt = ftl->status_q.head;
      ftl->status_q.head = elmt->next;
      free(elmt);
      ftl->status_q.count--;
    }

    os_unlock_mutex(&ftl->status_q.mutex);
    os_delete_mutex(&ftl->status_q.mutex);

    os_semaphore_delete(&ftl->status_q.sem);

    ingest_release(ftl);

    if (ftl->key != NULL) {
      free(ftl->key);
    }

    if (ftl->ingest_hostname != NULL) {
      free(ftl->ingest_hostname);
    }

  if (ftl->param_ingest_hostname != NULL) {
    free(ftl->param_ingest_hostname);
  }

    free(ftl);
  }

  return FTL_SUCCESS;
}

FTL_API ftl_status_t ftl_ingest_destroy(ftl_handle_t *ftl_handle){
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;

  ftl_handle->priv = NULL;

  return internal_ftl_ingest_destroy(ftl);
}

FTL_API char* ftl_status_code_to_string(ftl_status_t status) {

  switch (status) {
  case FTL_SUCCESS:
    return "Success";
  case FTL_SOCKET_NOT_CONNECTED:
    return "The socket is no longer connected";
  case FTL_MALLOC_FAILURE:
    return "Internal memory allocation error";
  case FTL_DNS_FAILURE:
    return "Failed to get an ip address for the specified ingest (DNS lookup failure)";
  case FTL_CONNECT_ERROR:
    return "An unknown error occurred connecting to the socket";
  case FTL_INTERNAL_ERROR:
    return "An Internal error occurred";
  case FTL_CONFIG_ERROR:
    return "The parameters supplied are invalid or incomplete";
  case FTL_STREAM_REJECTED:
    return "The Ingest rejected the stream";
  case FTL_NOT_ACTIVE_STREAM:
    return "The stream is not active";
  case FTL_UNAUTHORIZED:
    return "This channel is not authorized to connect to this ingest";
  case FTL_AUDIO_SSRC_COLLISION:
    return "The Audio SSRC is already in use";
  case FTL_VIDEO_SSRC_COLLISION:
    return "The Video SSRC is already in use";
  case FTL_BAD_REQUEST:
    return "A request to the ingest was invalid";
  case FTL_OLD_VERSION:
    return "The current version of the FTL-SDK is no longer supported";
  case FTL_BAD_OR_INVALID_STREAM_KEY:
    return "Invalid stream key";
  case FTL_UNSUPPORTED_MEDIA_TYPE:
    return "The specified media type is not supported";
  case FTL_NOT_CONNECTED:
    return "The channel is not connected";
  case FTL_ALREADY_CONNECTED:
    return "The channel is already connected";
  case FTL_STATUS_TIMEOUT:
    return "Timed out waiting for status message";
  case FTL_QUEUE_FULL:
    return "The status queue is full";
  case FTL_STATUS_WAITING_FOR_KEY_FRAME:
    return "dropping packets until a key frame is received";
  case FTL_QUEUE_EMPTY:
    return "The status queue is empty";
  case FTL_NOT_INITIALIZED:
    return "The parameters were not correctly initialized";
  case FTL_CHANNEL_IN_USE:
    return "Channel is already actively streaming";
  case FTL_REGION_UNSUPPORTED:
    return "The location you are attempting to stream from is not authorized to do so by the local government";
  case FTL_NO_MEDIA_TIMEOUT:
    return "The ingest did not receive any audio or video media for an extended period of time";
  case FTL_USER_DISCONNECT:
    return "ftl ingest disconnect api was called";
  case FTL_INGEST_NO_RESPONSE:
    return "ingest did not respond to request";
  case FTL_NO_PING_RESPONSE:
    return "ingest did not respond to keepalive";
  case FTL_SPEED_TEST_ABORTED:
    return "the speed test was aborted, possibly due to a network interruption";
  case FTL_INGEST_SOCKET_CLOSED:
    return "the ingest socket was closed";
  case FTL_INGEST_SOCKET_TIMEOUT:
    return "the ingest socket was hit a timeout.";
  case FTL_UNKNOWN_ERROR_CODE:
  default:
    /* Unknown FTL error */
    return "Unknown status code"; 
  }
}

BOOL _get_chan_id_and_key(const char *stream_key, uint32_t *chan_id, char *key) {
  size_t len, i = 0;
  
  len = strlen(stream_key);
  for (i = 0; i != len; i++) {
    /* find the comma that divides the stream key */
    if (stream_key[i] == '-' || stream_key[i] == ',') {
      /* stream key gets copied */
      strcpy_s(key, MAX_KEY_LEN, stream_key+i+1);

      /* Now get the channel id */
      char * copy_of_key = _strdup(stream_key);
      copy_of_key[i] = '\0';
      *chan_id = atol(copy_of_key);
      free(copy_of_key);

      return TRUE;
    }
  }

    return FALSE;
}


