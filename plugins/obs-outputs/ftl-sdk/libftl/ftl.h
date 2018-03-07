/**
* \file ftl.h - Public Interface for the FTL SDK
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

#ifndef __FTL_H
#define __FTL_H

#include <stdint.h>

#ifdef _WIN32
#  ifdef FTL_STATIC_COMPILE
#    define FTL_API
#  else
#    ifdef __FTL_INTERNAL
#      define FTL_API __declspec(dllexport)
#    else
#      define FTL_API __declspec(dllimport)
#    endif
#  endif
#else
#  define FTL_API
#endif

FTL_API extern const int FTL_VERSION_MAJOR;
FTL_API extern const int FTL_VERSION_MINOR;
FTL_API extern const int FTL_VERSION_MAINTENANCE;


/*! \defgroup ftl_public Public Interfaces for libftl */

/*! \brief Status codes used by libftl
*  \ingroup ftl_public
*/

typedef enum {
  FTL_SUCCESS,                  /**< Operation was successful */
  FTL_SOCKET_NOT_CONNECTED,
  FTL_NON_ZERO_POINTER,         /**< Function required a zero-ed pointer, but didn't get one */
  FTL_MALLOC_FAILURE,           /**< memory allocation failed */
  FTL_DNS_FAILURE,              /**< DNS probe failed */
  FTL_CONNECT_ERROR,            /**< Failed to connect to ingest */
  FTL_INTERNAL_ERROR,           /**< Got valid inputs, but FTL failed to complete the action due to internal failure */
  FTL_CONFIG_ERROR,             /**< The configuration supplied was invalid or incomplete */
  FTL_STREAM_REJECTED,          /**< Ingest rejected our connect command */
  FTL_NOT_ACTIVE_STREAM,        /**< The function required an active stream and was passed an inactive one */
  FTL_UNAUTHORIZED,             /**< Parameters were correct, but streamer not authorized to use FTL */
  FTL_AUDIO_SSRC_COLLISION,     /**< The audio SSRC from this IP is currently in use */
  FTL_VIDEO_SSRC_COLLISION,     /**< The video SSRC from this IP is currently in use */
  FTL_BAD_REQUEST,              /**< Ingest didn't like our request. Should never happen */
  FTL_OLD_VERSION,              /**< libftl needs to be updated */
  FTL_BAD_OR_INVALID_STREAM_KEY,
  FTL_UNSUPPORTED_MEDIA_TYPE,
  FTL_GAME_BLOCKED,             /**< The current game set by this profile can't be streamed. */
  FTL_NOT_CONNECTED,
  FTL_ALREADY_CONNECTED,
  FTL_UNKNOWN_ERROR_CODE,
  FTL_STATUS_TIMEOUT,
  FTL_QUEUE_FULL,
  FTL_STATUS_WAITING_FOR_KEY_FRAME,
  FTL_QUEUE_EMPTY,
  FTL_NOT_INITIALIZED,
  FTL_CHANNEL_IN_USE,           /**< The channel is already actively streaming */
  FTL_REGION_UNSUPPORTED,       /**< The region you are attempting to stream from is not authorized to stream by local governments */
  FTL_NO_MEDIA_TIMEOUT,
  FTL_USER_DISCONNECT,
  FTL_INGEST_NO_RESPONSE,
  FTL_NO_PING_RESPONSE,
  FTL_SPEED_TEST_ABORTED,
  FTL_INGEST_SOCKET_CLOSED,
  FTL_INGEST_SOCKET_TIMEOUT,
} ftl_status_t;

typedef enum {
  FTL_CONNECTION_DISCONNECTED,
  FTL_CONNECTION_RECONNECTED
} ftl_connection_status_t;


#define FOREVER -1
/*! \brief Video codecs supported by FTL
*  \ingroug ftl_public
*/

typedef enum {
  FTL_VIDEO_NULL, /**< No video for this stream */
  FTL_VIDEO_VP8,  /**< Google's VP8 codec (recommended default) */
  FTL_VIDEO_H264
} ftl_video_codec_t;

/*! \brief Audio codecs supported by FTL
*  \ingroup ftl_public
*/

typedef enum {
  FTL_AUDIO_NULL, /**< No audio for this stream */
  FTL_AUDIO_OPUS, /**< Xiph's Opus audio codec */
  FTL_AUDIO_AAC
} ftl_audio_codec_t;

typedef enum {
  FTL_AUDIO_DATA,
  FTL_VIDEO_DATA
} ftl_media_type_t;

/*! \brief Log levels used by libftl; returned via logging callback
*  \ingroup ftl_public
*/

typedef enum {
  FTL_LOG_CRITICAL,
  FTL_LOG_ERROR,
  FTL_LOG_WARN,
  FTL_LOG_INFO,
  FTL_LOG_DEBUG
} ftl_log_severity_t;

/*! \brief Function prototype for FTL logging callback
* \ingroup ftl_public
*/

typedef void(*ftl_logging_function_t)(ftl_log_severity_t log_level, const char * log_message);
typedef void(*ftl_status_function_t)(ftl_connection_status_t status);

typedef struct {
  char *ingest_hostname;
  char *stream_key;
  ftl_video_codec_t video_codec;
  ftl_audio_codec_t audio_codec;
  int peak_kbps; //used for the leaky bucket to smooth out packet flow rate, set to 0 to bypass
  int fps_num;
  int fps_den;
  char *vendor_name;
  char *vendor_version;
} ftl_ingest_params_t;

typedef struct {
  int pkts_sent;
  int nack_requests;
  int lost_pkts;
  int starting_rtt;
  int ending_rtt;
  int bytes_sent;
  int duration_ms;
  int peak_kbps;
}speed_test_t;

typedef struct {
  void* priv;
} ftl_handle_t;

typedef enum {
  FTL_STATUS_NONE,
  FTL_STATUS_LOG,
  FTL_STATUS_EVENT,
  FTL_STATUS_VIDEO_PACKETS,
  FTL_STATUS_VIDEO_PACKETS_INSTANT,
  FTL_STATUS_AUDIO_PACKETS,
  FTL_STATUS_VIDEO,
  FTL_STATUS_AUDIO,
  FTL_STATUS_FRAMES_DROPPED,
  FTL_STATUS_NETWORK,
  FTL_BITRATE_CHANGED
} ftl_status_types_t;

typedef enum {
  FTL_STATUS_EVENT_TYPE_UNKNOWN,
  FTL_STATUS_EVENT_TYPE_CONNECTED,
  FTL_STATUS_EVENT_TYPE_DISCONNECTED,
  FTL_STATUS_EVENT_TYPE_DESTROYED,
  FTL_STATUS_EVENT_INGEST_ERROR_CODE
} ftl_status_event_types_t;

typedef enum {
  FTL_STATUS_EVENT_REASON_NONE,
  FTL_STATUS_EVENT_REASON_NO_MEDIA,
  FTL_STATUS_EVENT_REASON_API_REQUEST,
  FTL_STATUS_EVENT_REASON_UNKNOWN,
} ftl_status_event_reasons_t;

typedef struct {
  int log_level;
  char string[1024];
}ftl_status_log_msg_t;

typedef struct {
  ftl_status_event_types_t type;
  ftl_status_event_reasons_t reason;
  ftl_status_t error_code;
}ftl_status_event_msg_t;

typedef struct {
  int64_t period; //period of time in ms the stats were collected over
  int64_t sent;
  int64_t nack_reqs;
  int64_t lost;
  int64_t recovered;
  int64_t late;
}ftl_packet_stats_msg_t;

typedef struct {
  int64_t period; //period of time in ms the stats were collected over
  int min_rtt;
  int max_rtt;
  int avg_rtt;
  int min_xmit_delay;
  int max_xmit_delay;
  int avg_xmit_delay;
}ftl_packet_stats_instant_msg_t;

typedef struct {
  int64_t period; //period of time in ms the stats were collected over
  int64_t frames_queued;
  int64_t frames_sent;
  int64_t bytes_queued;
  int64_t bytes_sent;
  int64_t bw_throttling_count;
  int queue_fullness;
  int max_frame_size;
}ftl_video_frame_stats_msg_t;

typedef enum
{
    FTL_BITRATE_DECREASED,
    FTL_BITRATE_INCREASED,
    FTL_BITRATE_STABILIZED
}ftl_bitrate_changed_type_t;

typedef enum
{
    FTL_BANDWIDTH_CONSTRAINED,
    FTL_UPGRADE_EXCESSIVE,
    FTL_BANDWIDTH_AVAILABLE,
    FTL_STABILIZE_ON_LOWER_BITRATE,
    FTL_STABILIZE_ON_ORIGINAL_BITRATE,
} ftl_bitrate_changed_reason_t;

typedef struct
{
    ftl_bitrate_changed_type_t bitrate_changed_type;
    ftl_bitrate_changed_reason_t bitrate_changed_reason;
    uint64_t current_encoding_bitrate;
    uint64_t previous_encoding_bitrate;
    float nacks_to_frames_ratio;
    float avg_rtt;
    uint64_t avg_frames_dropped;
    float queue_fullness;
} ftl_bitrate_changed_msg_t;

/*status messages*/
typedef struct
{
    ftl_status_types_t type;
    union
    {
        ftl_status_log_msg_t log;
        ftl_status_event_msg_t event;
        ftl_packet_stats_msg_t pkt_stats;
        ftl_packet_stats_instant_msg_t ipkt_stats;
        ftl_video_frame_stats_msg_t video_stats;
        ftl_bitrate_changed_msg_t bitrate_changed_msg;
    } msg;
}ftl_status_msg_t;

/*!
* \ingroup ftl_public
* \brief FTL Initialization
*
* Before using FTL, you must call ftl_init before making any additional calls
* in this library. ftl_init initializes any submodules that FTL depends on such
* as libsrtp. Under normal cirmstances, this function should never fail.
*
* On Windows, this function calls WSAStartup() to initialize Winsock. It is
* the responsibility of the calling app to call WSACleanup() at application
* shutdown as FTL can't safely call it (as your application may be using sockets
* elsewhere
*
* @returns FTL_INIT_SUCCESS on successful initialization. Otherwise, returns
* ftl_init_status_t enum with the failure state.
*/
FTL_API ftl_status_t ftl_init();

FTL_API ftl_status_t ftl_ingest_create(ftl_handle_t *ftl_handle, ftl_ingest_params_t *params);

FTL_API ftl_status_t ftl_ingest_connect(ftl_handle_t *ftl_handle);

FTL_API int ftl_ingest_speed_test(ftl_handle_t *ftl_handle, int speed_kbps, int duration_ms);
FTL_API ftl_status_t ftl_ingest_speed_test_ex(ftl_handle_t *ftl_handle, int speed_kbps, int duration_ms, speed_test_t *results);

// Deprecated! Please use the DTS version.
FTL_API int ftl_ingest_send_media(ftl_handle_t *ftl_handle, ftl_media_type_t media_type, uint8_t *data, int32_t len, int end_of_frame);

FTL_API int ftl_ingest_send_media_dts(ftl_handle_t *ftl_handle, ftl_media_type_t media_type, int64_t dts_usec, uint8_t *data, int32_t len, int end_of_frame);

FTL_API ftl_status_t ftl_ingest_get_status(ftl_handle_t *ftl_handle, ftl_status_msg_t *msg, int ms_timeout);

FTL_API ftl_status_t ftl_ingest_update_params(ftl_handle_t *ftl_handle, ftl_ingest_params_t *params);

FTL_API ftl_status_t ftl_ingest_disconnect(ftl_handle_t *ftl_handle);

FTL_API ftl_status_t ftl_ingest_destroy(ftl_handle_t *ftl_handle);

FTL_API char* ftl_status_code_to_string(ftl_status_t status);

FTL_API ftl_status_t ftl_get_video_stats(ftl_handle_t* handle, uint64_t* frames_sent, uint64_t* nacks_received, uint64_t* rtt_recorded, uint64_t* frames_dropped, float* queue_fullness);

FTL_API ftl_status_t ftl_adaptive_bitrate_thread(
    ftl_handle_t* ftl_handle,
    void* context,
    int(*change_bitrate_callback)(void*, uint64_t),
    uint64_t initial_encoding_bitrate,
    uint64_t min_encoding_bitrate,
    uint64_t max_encoding_bitrate
);


#endif // __FTL_H
