/**
 * ftl_helpers.c -
 *
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

#include <ctype.h>
#include "ftl.h"
#include "ftl_private.h"
#include "hmac/hmac.h"

/*
    Please note that throughout the code, we send "\r\n\r\n", where a normal newline ("\n") would suffice.
    This is done due to some firewalls / anti-malware systems not passing our packets through when we don't send those double-windows-newlines.
    They seem to incorrectly detect our protocol as HTTP.
*/

int ftl_read_response_code(const char * response_str) {
  int response_code = 0;
  int count;

  count = sscanf_s(response_str, "%d", &response_code);

  return count ? response_code : -1;

 }

int ftl_read_media_port(const char *response_str) {
  int port = -1;

  if ((sscanf_s(response_str, "%*[^.]. Use UDP port %d\n", &port)) != 1) {
    return -1;
  }

  return port;
}

unsigned char decode_hex_char(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    // Set the 5th bit. Makes ASCII chars lowercase :)
    c |= (1 << 5);

    if (c >= 'a' && c <= 'z') {
        return (c - 'a') + 10;
    }

    return 0;
}

int recv_all(SOCKET sock, char * buf, int buflen, const char line_terminator) {
    int pos = 0;
    int n;
    int bytes_recd = 0;

    do {
        n = recv(sock, buf, buflen, 0);
        if (n < 0) {
            //this will abort in the event of an error or in the buffer is filled before the terminator is reached
            return n;
        }
        else if (n == 0) {
            // The socket is closed.
            return 0;
        }

        buf += n;
        buflen -= n;
        bytes_recd += n;
    } while(buf[-1] != line_terminator);

  buf[bytes_recd] = '\0';

    return bytes_recd;
}

int ftl_get_hmac(SOCKET sock, char * auth_key, char * dst) {
    char buf[2048];
    int string_len;
    int response_code;

    send(sock, "HMAC\r\n\r\n", 8, 0);
    string_len = recv_all(sock, buf, 2048, '\n');
    if (string_len < 4 || string_len == 2048) {
        return 0;
    }

    response_code = ftl_read_response_code(buf);
    if (response_code != FTL_INGEST_RESP_OK) {
        return 0;
    }

    int len = string_len - 5; // Strip "200 " and "\n"
    if (len % 2) {
        return 0;
    }

    int messageLen = len / 2;
    unsigned char *msg;

    if( (msg = (unsigned char*)malloc(messageLen * sizeof(*msg))) == NULL){
        return 0;
    }

    int i;
    const char *hexMsgBuf = buf + 4;
    for(i = 0; i < messageLen; i++) {
        msg[i] = (decode_hex_char(hexMsgBuf[i * 2]) << 4) +
                  decode_hex_char(hexMsgBuf[(i * 2) + 1]);
    }

    hmacsha512(auth_key, msg, messageLen, dst);
    free(msg);
    return 1;
}

const char * ftl_audio_codec_to_string(ftl_audio_codec_t codec) {
  switch (codec) {
    case FTL_AUDIO_NULL: return "";
    case FTL_AUDIO_OPUS: return "OPUS";
    case FTL_AUDIO_AAC: return "AAC";
  }

  // Should be never reached
  return "";
}

const char * ftl_video_codec_to_string(ftl_video_codec_t codec) {
  switch (codec) {
    case FTL_VIDEO_NULL: return "";
    case FTL_VIDEO_VP8: return "VP8";
    case FTL_VIDEO_H264: return "H264";
  }

  // Should be never reached
  return "";
}

void ftl_set_state(ftl_stream_configuration_private_t *ftl, ftl_state_t state) {
  os_lock_mutex(&ftl->state_mutex);
  ftl->state |= state;
  os_unlock_mutex(&ftl->state_mutex);
}

void ftl_clear_state(ftl_stream_configuration_private_t *ftl, ftl_state_t state) {
  os_lock_mutex(&ftl->state_mutex);
  ftl->state &= ~state;
  os_unlock_mutex(&ftl->state_mutex);
}

BOOL ftl_get_state(ftl_stream_configuration_private_t *ftl, ftl_state_t state) {
  BOOL is_set;

  os_lock_mutex(&ftl->state_mutex);
  is_set = ftl->state & state;
  os_unlock_mutex(&ftl->state_mutex);

  return is_set;

}

BOOL is_legacy_ingest(ftl_stream_configuration_private_t *ftl) {
  return ftl->media.assigned_port == FTL_UDP_MEDIA_PORT;
}

ftl_status_t enqueue_status_msg(ftl_stream_configuration_private_t *ftl, ftl_status_msg_t *stats_msg) {
  status_queue_elmt_t *elmt;
  ftl_status_t retval = FTL_SUCCESS;

  os_lock_mutex(&ftl->status_q.mutex);

  if ( (elmt = (status_queue_elmt_t*)malloc(sizeof(status_queue_elmt_t))) == NULL) {
    fprintf(stderr, "Unable to allocate status msg");
    return FTL_MALLOC_FAILURE;
  }

  memcpy(&elmt->stats_msg, stats_msg, sizeof(status_queue_elmt_t));
  elmt->next = NULL;

  if (ftl->status_q.head == NULL) {
    ftl->status_q.head = elmt;
  }
  else {
    status_queue_elmt_t *tail = ftl->status_q.head;

    do {
      if (tail->next == NULL) {
        tail->next = elmt;
        break;
      }
      tail = tail->next;
    } while (tail != NULL);
  }

  /*if queue is full remove head*/
  if (ftl->status_q.count >= MAX_STATUS_MESSAGE_QUEUED) {
    elmt = ftl->status_q.head;
    ftl->status_q.head = elmt->next;
    free(elmt);
    retval = FTL_QUEUE_FULL;
  }
  else {
    ftl->status_q.count++;
    os_semaphore_post(&ftl->status_q.sem);
  }

  os_unlock_mutex(&ftl->status_q.mutex);
  return retval;
}

ftl_status_t dequeue_status_msg(ftl_stream_configuration_private_t *ftl, ftl_status_msg_t *stats_msg, int ms_timeout) {
  status_queue_elmt_t *elmt;
  ftl_status_t retval = FTL_SUCCESS;

  if (!ftl_get_state(ftl, FTL_STATUS_QUEUE)) {
    return FTL_NOT_INITIALIZED;
  }

  ftl->status_q.thread_waiting = 1;

  if (os_semaphore_pend(&ftl->status_q.sem, ms_timeout) < 0) {
    return FTL_STATUS_TIMEOUT;
  }

  os_lock_mutex(&ftl->status_q.mutex);

  if (ftl->status_q.head != NULL) {
    elmt = ftl->status_q.head;
    memcpy(stats_msg, &elmt->stats_msg, sizeof(elmt->stats_msg));
    ftl->status_q.head = elmt->next;
    free(elmt);
    ftl->status_q.count--;
  }
  else {
    retval = FTL_QUEUE_EMPTY;
  }

  os_unlock_mutex(&ftl->status_q.mutex);

  ftl->status_q.thread_waiting = 0;

  return retval;
}

ftl_status_t _set_ingest_hostname(ftl_stream_configuration_private_t *ftl) {

  ftl_status_t ret_status = FTL_SUCCESS;

  do {
#ifndef DISABLE_AUTO_INGEST
      if (strcmp(ftl->param_ingest_hostname, "auto") == 0) {
      ftl->ingest_hostname = ingest_find_best(ftl);
      }
      else
#endif
    ftl->ingest_hostname = _strdup(ftl->param_ingest_hostname);
  } while (0);

  return ret_status;
}

int _get_remote_ip(struct sockaddr *addr, size_t addrlen, char *remote_ip, size_t ip_len) {
  if (addr->sa_family == AF_INET)
  {
    struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)addr;

    if (inet_ntop(AF_INET, &ipv4_addr->sin_addr.s_addr, remote_ip, ip_len) == NULL) {
      return -1;
    }
  }
  else if (addr->sa_family == AF_INET6) {
    struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6 *)addr;

    if (inet_ntop(AF_INET6, &ipv6_addr->sin6_addr.s6_addr, remote_ip, ip_len) == NULL) {
      return -1;
    }
  }

  return 0;
}
