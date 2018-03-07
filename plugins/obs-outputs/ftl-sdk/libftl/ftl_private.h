/**
 * \file ftl_private.h - Private Interfaces for the FTL SDK
 *
 * Copyright (c) 2015 Mixer Inc.
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

#ifndef __FTL_PRIVATE_H
#define __FTL_PRIVATE_H

#define __STDC_WANT_LIB_EXT1__ 1

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gettimeofday/gettimeofday.h"

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include "threads.h"
#include "socket.h"


#define MAX_INGEST_COMMAND_LEN 512
#define INGEST_PORT 8084
#define MAX_KEY_LEN 100
#define VIDEO_PTYPE 96
#define AUDIO_PTYPE 97
#define SOCKET_RECV_TIMEOUT_MS 5000
#define SOCKET_SEND_TIMEOUT_MS 1000
#define KEEPALIVE_FREQUENCY_MS 5000
#define KEEPALIVE_SEND_WARN_TOLERANCE_MS 1000
#define STATUS_THREAD_SLEEP_TIME_MS 500
#define MAX_PACKET_BUFFER 1500  //Max length of buffer
#define MAX_MTU 1392
#define FTL_UDP_MEDIA_PORT 8082   //legacy port
#define RTP_HEADER_BASE_LEN 12
#define RTP_FUA_HEADER_LEN 2
#define NACK_RB_SIZE (2048) //must be evenly divisible by 2^16
#define NACK_RTT_AVG_SECONDS 5
#define MAX_STATUS_MESSAGE_QUEUED 10
#define MAX_FRAME_SIZE_ELEMENTS 64 //must be a minimum of 3
#define MAX_XMIT_LEVEL_IN_MS 100 //allows a maximum burst size of 100ms at the target bitrate
#define VIDEO_RTP_TS_CLOCK_HZ 90000
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_PACKET_DURATION_MS 20
#define IPVX_ADDR_ASCII_LEN INET6_ADDRSTRLEN
#define INGEST_LIST_URI "https://mixer.com/api/v1/ingests/best"
#define INGEST_LOAD_PORT 8079
#define INGEST_PING_PORT 8079
#define PEAK_BITRATE_KBPS 10000 /*if not supplied this is the peak from the perspective of the send buffer*/
#define PING_TX_INTERVAL_MS 25
#define SENDER_REPORT_TX_INTERVAL_MS 1000
#define PING_PTYPE 250
#define SENDER_REPORT_PTYPE 200

 // Adaptive bitrate constants

 // If the ratio of nacks received to packets sent is greater than the following value, we request a bitrate downgrade.
#define MIN_NACKS_RECEIVED_TO_PACKETS_SENT_RATIO_FOR_BITRATE_DOWNGRADE 0.1

 // Duration at which we capture stream stats , i.e frames sent and nacks received
#define STREAM_STATS_CAPTURE_MS 1000

 // Duration over which we evaluate whether we need to downgrade/upgrade bitrate. Note that we 
 // look at stats over the last c_ulBwCheckDurationMs milliseconds, every c_ulStreamStatsCaptureMs milliseconds.
#define BW_CHECK_DURATION_MS 5000

 // Interval to wait for bw test after we update bitrate.
#define BITRATE_CHANGED_COOLDOWN_INTERVAL_MS 10000

 // Percentage to reduce the bitrate to if bandwidth seems too constrained
#define BW_INSUFFICIENT_BITRATE_DOWNGRADE_PERCENTAGE 50

 // Percentange to reduce the bitrate to if bw upgrade was too excessive
#define REVERT_TO_STABLE_BITRATE_DOWNGRADE_PERCENTAGE 80

 // Percentage to increase bitrate by if conditions look ideal
#define BW_IDEAL_BITRATE_UPGRADE_BPS 256000;

 // If ratio of nacks received to packets sent is below the following value bitrate update can be requested
#define MAX_NACKS_RECEIVED_TO_PACKETS_SENT_RATIO_FORBITRATE_UPGRADE 0.01

 // If bandwidth is constrained within MaxBitrateUpgradeExcessiveSeconds after bitrate update, revert to last stable bitrate.
#define MAX_MS_TO_DEEM_UPGRADE_EXCESSIVE 60000

#define MAX_AVG_RTT_TO_DEEM_BW_STABLE 100

#define MAX_QUEUE_FULLNESS_TO_DEEM_BW_STABLE 0.1

#define MIN_QUEUE_FULLNESS_TO_DEEM_BW_CONSTRAINED 0.3

#define MIN_AVG_RTT_TO_DEEM_BW_CONSTRAINED 300
 // If bitrate upgrade was excessive we freeze bitrate upgrade for the next c_bitrateUpgradeFreezeTimeMs milliseconds.
#define BITRATE_UPGRADE_FREEZE_TIME_MS 600000 // 10*60*1000

#define MAX_STAT_SIZE 5

#ifndef _WIN32
#define strncpy_s(dst, dstsz, src, cnt) strncpy(dst, src, cnt)
#define sprintf_s(buf, bufsz, fmt, ...) sprintf(buf, fmt, __VA_ARGS__)
#define strcpy_s(dst, dstsz, src) strcpy(dst, src)
#define _strdup(src) strdup(src)
#define sscanf_s sscanf
#endif

typedef enum {
  H264_NALU_TYPE_NON_IDR = 1,
  H264_NALU_TYPE_IDR = 5,
  H264_NALU_TYPE_SEI = 6,
  H264_NALU_TYPE_SPS = 7,
  H264_NALU_TYPE_PPS = 8,
  H264_NALU_TYPE_DELIM = 9,
  H264_NALU_TYPE_FILLER = 12
}h264_nalu_type_t;

typedef enum {
  FTL_CONNECTED = 0x0001,
  FTL_MEDIA_READY = 0x0002,
  FTL_STATUS_QUEUE = 0x0004,
  FTL_CXN_STATUS_THRD = 0x0008,
  FTL_KEEPALIVE_THRD = 0x0010,
  FTL_PING_THRD = 0x0020,
  FTL_RX_THRD = 0x0040,
  FTL_TX_THRD = 0x0080,
  FTL_DISABLE_TX_PING_PKTS = 0x0100,
  FTL_SPEED_TEST = 0x0200,
  FTL_BITRATE_THRD = 0x0400,
  FTL_DISCONNECT_IN_PROGRESS = 0x1000,
  FTL_DISABLE_TX_SENDER_REPORT = 0x2000
}ftl_state_t;

#ifndef _WIN32
typedef int SOCKET;
typedef bool BOOL;
#define TRUE true
#define FALSE false
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif

/*status message queue*/
typedef struct _status_queue_t {
  ftl_status_msg_t stats_msg;
  struct _status_queue_t *next;
}status_queue_elmt_t;

typedef struct {
  status_queue_elmt_t *head;
  int count;
  int thread_waiting;
  OS_MUTEX mutex;
  OS_SEMAPHORE sem;
}status_queue_t;

/**
 * This configuration structure handles basic information for a struct such
 * as the authetication keys and other similar information. It's members are
 * private and not to be directly manipulated
 */
typedef struct {
  uint8_t packet[MAX_PACKET_BUFFER];
  int len;
  struct timeval insert_time;
  struct timeval xmit_time;
  int sn;
  int first;/*first packet in frame*/
  int last; /*last packet in frame*/
  OS_MUTEX mutex;
  BOOL isPartOfIframe;
}nack_slot_t;

typedef struct _ping_pkt_t {
  uint32_t header;
  struct timeval xmit_time;
}ping_pkt_t;

typedef struct _senderReport_pkt_t {
  uint32_t header;
  uint32_t ssrc;
  uint32_t ntpTimestampHigh;
  uint32_t ntpTimestampLow;
  uint32_t rtpTimestamp;
  uint32_t senderPacketCount;
  uint32_t senderOctetCount;
}senderReport_pkt_t;

typedef struct {
  struct timeval start_time;
  int64_t frames_received;
  int64_t frames_sent;
  int64_t bw_throttling_count;
  int64_t bytes_queued;
  int64_t packets_queued;
  int64_t bytes_sent;
  int64_t payload_bytes_sent;
  int64_t packets_sent;
  int64_t late_packets;
  int64_t lost_packets;
  int64_t nack_requests;
  int64_t dropped_frames;
  int pkt_xmit_delay_max;
  int pkt_xmit_delay_min;
  int total_xmit_delay;
  int xmit_delay_samples;
  int pkt_rtt_max;
  int pkt_rtt_min;
  int total_rtt;
  int rtt_samples;
  int current_frame_size;
  int max_frame_size;
}media_stats_t;

typedef struct {
  uint8_t payload_type;
  uint32_t ssrc;
  uint32_t timestamp;
  int timestamp_clock;
  uint64_t timestamp_dts_usec;
  int64_t base_dts_usec;
  int64_t randomOffset;
  uint16_t seq_num;
  uint16_t tmp_seq_num; // used for stats only
  BOOL nack_enabled;
  int64_t min_nack_rtt;
  int64_t max_nack_rtt;
  int64_t nack_rtt_avg;
  BOOL nack_slots_initalized;
  int producer;
  int consumer;
  uint16_t xmit_seq_num;
  nack_slot_t *nack_slots[NACK_RB_SIZE];
  OS_MUTEX nack_slots_lock;
  int peak_kbps;
  int kbps;
  media_stats_t stats; //cumulative since start of stream
  OS_SEMAPHORE pkt_ready;
}ftl_media_component_common_t;

typedef struct {
  ftl_audio_codec_t codec;
  int64_t dts_usec;
  ftl_media_component_common_t media_component;
  OS_MUTEX mutex;
  BOOL is_ready_to_send;
} ftl_audio_component_t;

typedef struct {
  ftl_video_codec_t codec;
  uint32_t height;
  uint32_t width;
  int fps_num;
  int fps_den;
  int64_t dts_usec;
  float dts_error;
  uint8_t fua_nalu_type;
  BOOL wait_for_idr_frame;
  ftl_media_component_common_t media_component;
  OS_MUTEX mutex;
  BOOL has_sent_first_frame;
} ftl_video_component_t;

typedef struct {
  size_t ingest_addrlen;
  struct sockaddr *ingest_addr;
  SOCKET media_socket;
  OS_MUTEX mutex;
  int assigned_port;
  OS_THREAD_HANDLE recv_thread;
  OS_THREAD_HANDLE video_send_thread;
  OS_THREAD_HANDLE audio_send_thread;
  OS_THREAD_HANDLE ping_thread;
  OS_SEMAPHORE ping_thread_shutdown;
  int max_mtu;
  struct timeval stats_tv;
  int last_rtt_delay;
  struct timeval sender_report_base_ntp;
} ftl_media_config_t;

typedef struct _ftl_ingest_t {
  char *name;
  char *ip;
  char *hostname;
  int rtt;
  struct _ftl_ingest_t *next;
}ftl_ingest_t;

typedef struct
{
    ftl_handle_t* handle;
    BOOL(*change_bitrate_callback)(void*, uint64_t);
    void* context;
    uint64_t initial_encoding_bitrate;
    uint64_t max_encoding_bitrate;
    uint64_t min_encoding_bitrate;
} ftl_adaptive_bitrate_thread_params_t;

typedef struct {
  SOCKET ingest_socket;
  ftl_state_t state;
  OS_MUTEX state_mutex;
  OS_MUTEX disconnect_mutex;
  char *param_ingest_hostname;
  char *ingest_hostname;
  char *ingest_ip;
  short socket_family;
  uint32_t channel_id;
  char *key;
  char hmacBuffer[512];
  int video_kbps;
  char vendor_name[20];
  char vendor_version[20];
  OS_THREAD_HANDLE connection_thread;
  OS_THREAD_HANDLE keepalive_thread;
  OS_THREAD_HANDLE bitrate_monitor_thread;
  OS_SEMAPHORE connection_thread_shutdown;
  OS_SEMAPHORE keepalive_thread_shutdown;
  OS_SEMAPHORE bitrate_thread_shutdown;
  ftl_media_config_t media;
  ftl_audio_component_t audio;
  ftl_video_component_t video;
  status_queue_t status_q;
  ftl_ingest_t *ingest_list;
  int ingest_count;
}  ftl_stream_configuration_private_t;

struct MemoryStruct {
  char *memory;
  size_t size;
};

/**
 * Charon always responses with a three digit response code after each command
 *
 * This enum holds defined number sequences
 **/

typedef enum {
  FTL_INGEST_RESP_UNKNOWN = 0,
  FTL_INGEST_RESP_OK = 200,
  FTL_INGEST_RESP_PING = 201,
  FTL_INGEST_RESP_BAD_REQUEST= 400,             // The handshake was not formatted correctly
  FTL_INGEST_RESP_UNAUTHORIZED = 401,           // This channel id is not authorized to stream
  FTL_INGEST_RESP_OLD_VERSION = 402,            // This ftl api version is no longer supported
  FTL_INGEST_RESP_AUDIO_SSRC_COLLISION = 403,
  FTL_INGEST_RESP_VIDEO_SSRC_COLLISION = 404,
  FTL_INGEST_RESP_INVALID_STREAM_KEY = 405,     // The corresponding channel does not match this key
  FTL_INGEST_RESP_CHANNEL_IN_USE = 406,         // The channel ID successfully authenticated however it is already actively streaming
  FTL_INGEST_RESP_REGION_UNSUPPORTED = 407,     // Streaming from this country or region is not authorized by local governments
  FTL_INGEST_RESP_NO_MEDIA_TIMEOUT = 408,
  FTL_INGEST_RESP_GAME_BLOCKED = 409,           // The game the user account is set to can't be streamed.
  FTL_INGEST_RESP_INTERNAL_SERVER_ERROR = 500,
  FTL_INGEST_RESP_INTERNAL_MEMORY_ERROR = 900,
  FTL_INGEST_RESP_INTERNAL_COMMAND_ERROR = 901,
  FTL_INGEST_RESP_INTERNAL_SOCKET_CLOSED = 902,
  FTL_INGEST_RESP_INTERNAL_SOCKET_TIMEOUT = 903,
} ftl_response_code_t;

/**
 * Logs something to the FTL logs
 */

#define FTL_LOG(ftl_handle, log_level, ...) ftl_log_msg (ftl_handle, log_level, __FILE__, __LINE__, __VA_ARGS__);
void ftl_log_msg(ftl_stream_configuration_private_t *ftl, ftl_log_severity_t log_level, const char * file, int lineno, const char * fmt, ...);

/**
 * Value to string conversion functions
 */

const char * ftl_audio_codec_to_string(ftl_audio_codec_t codec);
const char * ftl_video_codec_to_string(ftl_video_codec_t codec);

/**
 * Functions related to the charon prootocol itself
 **/

int recv_all(SOCKET sock, char * buf, int buflen, const char line_terminator);

int ftl_get_hmac(SOCKET sock, char * auth_key, char * dst);
int ftl_read_response_code(const char * response_str);
int ftl_read_media_port(const char *response_str);

/**
 * Platform abstractions
 **/

// FIXME: make this less global
extern char error_message[1000];

ftl_status_t _log_response(ftl_stream_configuration_private_t *ftl, int response_code);
void ftl_set_state(ftl_stream_configuration_private_t *ftl, ftl_state_t state);
void ftl_clear_state(ftl_stream_configuration_private_t *ftl, ftl_state_t state);
BOOL ftl_get_state(ftl_stream_configuration_private_t *ftl, ftl_state_t state);
BOOL is_legacy_ingest(ftl_stream_configuration_private_t *ftl);
ftl_status_t dequeue_status_msg(ftl_stream_configuration_private_t *ftl, ftl_status_msg_t *stats_msg, int ms_timeout);
ftl_status_t enqueue_status_msg(ftl_stream_configuration_private_t *ftl, ftl_status_msg_t *stats_msg);
ftl_status_t _set_ingest_hostname(ftl_stream_configuration_private_t *ftl);
int _get_remote_ip(struct sockaddr *addr, size_t addrlen, char *remote_ip, size_t ip_len);

ftl_status_t _init_control_connection(ftl_stream_configuration_private_t *ftl);
ftl_status_t _ingest_connect(ftl_stream_configuration_private_t *stream_config);
ftl_status_t _ingest_disconnect(ftl_stream_configuration_private_t *stream_config);
char * ingest_find_best(ftl_stream_configuration_private_t *ftl);
void ingest_release(ftl_stream_configuration_private_t *ftl);

ftl_status_t media_init(ftl_stream_configuration_private_t *ftl);
ftl_status_t media_destroy(ftl_stream_configuration_private_t *ftl);
int media_send_video(ftl_stream_configuration_private_t *ftl, int64_t dts_usec, uint8_t *data, int32_t len, int end_of_frame);
int media_send_audio(ftl_stream_configuration_private_t *ftl, int64_t dts_usec, uint8_t *data, int32_t len);
ftl_status_t media_speed_test(ftl_stream_configuration_private_t *ftl, int speed_kbps, int duration_ms, speed_test_t *results);
ftl_status_t internal_ingest_disconnect(ftl_stream_configuration_private_t *ftl);
ftl_status_t internal_ftl_ingest_destroy(ftl_stream_configuration_private_t *ftl);
void sleep_ms(int ms);

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#endif
