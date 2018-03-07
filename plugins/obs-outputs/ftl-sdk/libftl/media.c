#define __FTL_INTERNAL
#include "ftl.h"
#include "ftl_private.h"
#include <assert.h>

#define MAX_RTT_FACTOR 1.3
#define USEC_IN_SEC 1000000

OS_THREAD_ROUTINE video_send_thread(void *data);
OS_THREAD_ROUTINE audio_send_thread(void *data);
OS_THREAD_ROUTINE recv_thread(void *data);
OS_THREAD_ROUTINE ping_thread(void *data);
OS_THREAD_ROUTINE adaptive_bitrate_thread(void* data);
ftl_status_t _internal_media_destroy(ftl_stream_configuration_private_t *ftl);
static int _nack_init(ftl_media_component_common_t *media);
static int _nack_destroy(ftl_media_component_common_t *media);
static ftl_media_component_common_t *_media_lookup(ftl_stream_configuration_private_t *ftl, uint32_t ssrc);
static int _media_make_video_rtp_packet(ftl_stream_configuration_private_t *ftl, uint8_t *in, int in_len, uint8_t *out, int *out_len, int first_pkt);
static int _media_make_audio_rtp_packet(ftl_stream_configuration_private_t *ftl, uint8_t *in, int in_len, uint8_t *out, int *out_len);
static int _media_set_marker_bit(ftl_media_component_common_t *mc, uint8_t *in);
static int _media_send_packet(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc);
static int _media_send_slot(ftl_stream_configuration_private_t *ftl, nack_slot_t *slot);
static nack_slot_t* _media_get_empty_slot(ftl_stream_configuration_private_t *ftl, uint32_t ssrc, uint16_t sn);
static float _media_get_queue_fullness(ftl_stream_configuration_private_t *ftl, uint32_t ssrc);
void _update_timestamp(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc, int64_t dts_usec);
static void _update_xmit_level(ftl_stream_configuration_private_t *ftl, int *transmit_level, struct timeval *start_tv, int bytes_per_ms);

void _clear_stats(media_stats_t *stats);
static int _update_stats(ftl_stream_configuration_private_t *ftl);
static int _send_pkt_stats(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc, int interval_ms);
static int _send_video_stats(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc, int interval_ms);
static int _send_instant_pkt_stats(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc, int interval_ms);

size_t ingest_addrlen;
struct sockaddr *ingest_addr;

ftl_status_t _get_addr_info(short family, char *ip, short port, struct sockaddr **addr, size_t *addrlen) {

  ftl_status_t retval = FTL_SUCCESS;

    do {
      if (family == AF_INET) {
        struct sockaddr_in *ipv4_addr = NULL;
        size_t len;

        len = sizeof(struct sockaddr_in);

        if ((ipv4_addr = malloc(len)) == NULL) {
          retval = FTL_MALLOC_FAILURE;
          break;
        }

        memset(ipv4_addr, 0, len);

        ipv4_addr->sin_family = family;
        ipv4_addr->sin_port = htons(port);

        if (inet_pton(family, ip, &ipv4_addr->sin_addr) != 1) {
          retval = FTL_DNS_FAILURE;
          break;
        }

        *addrlen = len;
        *addr = (struct sockaddr *)ipv4_addr;
      }
      else if (family == AF_INET6) {
        struct sockaddr_in6 *ipv6_addr = NULL;
        size_t len;

        len = sizeof(struct sockaddr_in6);

        if ((ipv6_addr = malloc(len)) == NULL) {
                retval = FTL_MALLOC_FAILURE;
                break;
        }

        memset(ipv6_addr, 0, len);

        ipv6_addr->sin6_family = family;
        ipv6_addr->sin6_port = htons(port);

        if (inet_pton(family, ip, &ipv6_addr->sin6_addr) != 1) {
                retval = FTL_DNS_FAILURE;
                break;
        }

        *addrlen = len;
        *addr = (struct sockaddr *)ipv6_addr;
      }
  }
  while (0);
  return retval;
}

ftl_status_t media_init(ftl_stream_configuration_private_t *ftl) {

  ftl_media_config_t *media = &ftl->media;
  ftl_status_t status = FTL_SUCCESS;
  int idx;

  if (ftl_get_state(ftl, FTL_MEDIA_READY)) {
    return FTL_SUCCESS;
  }

  do {
    os_init_mutex(&media->mutex);
    os_init_mutex(&ftl->video.mutex);
    os_init_mutex(&ftl->audio.mutex);

    //use the same socket family as the control connection
    media->media_socket = socket(ftl->socket_family, SOCK_DGRAM, IPPROTO_UDP);
    if (media->media_socket == -1) {
            return FTL_DNS_FAILURE;
    }

    if ((status = _get_addr_info(ftl->socket_family, ftl->ingest_ip, media->assigned_port, &media->ingest_addr, &media->ingest_addrlen)) != FTL_SUCCESS) {
            return status;
    }

    media->max_mtu = MAX_MTU;
    gettimeofday(&media->stats_tv, NULL);
    media->sender_report_base_ntp.tv_usec = 0;
    media->sender_report_base_ntp.tv_sec = 0;

    ftl_media_component_common_t *media_comp[] = { &ftl->video.media_component, &ftl->audio.media_component };
    ftl_media_component_common_t *comp;

    for (idx = 0; idx < sizeof(media_comp) / sizeof(media_comp[0]); idx++) {

      comp = media_comp[idx];

      comp->nack_slots_initalized = FALSE;

      if ((status = _nack_init(comp)) != FTL_SUCCESS) {
        goto cleanup;
      }

      // According to RTP the time stamps should start at random values,
      // but to help sync issues and to make sync easier to calculate we
      // start at 0.
      comp->timestamp = 0;
      comp->producer = 0;
      comp->consumer = 0;
      comp->base_dts_usec = -1;

      _clear_stats(&comp->stats);
    }

    ftl->video.media_component.timestamp_clock = VIDEO_RTP_TS_CLOCK_HZ;
    ftl->audio.media_component.timestamp_clock = AUDIO_SAMPLE_RATE;
    ftl->audio.is_ready_to_send = FALSE;
    ftl->video.has_sent_first_frame = FALSE;

    ftl->video.wait_for_idr_frame = TRUE;

    // We need set this flag now so it is ready when the thread starts, but also
    // so it is set if we destroy this before the thread starts it will be cleaned up.
    ftl_set_state(ftl, FTL_RX_THRD);
    if ((os_create_thread(&media->recv_thread, NULL, recv_thread, ftl)) != 0) {
      ftl_clear_state(ftl, FTL_RX_THRD);
      status = FTL_MALLOC_FAILURE;
      break;
    }

    if (os_semaphore_create(&ftl->video.media_component.pkt_ready, "/VideoPkt", O_CREAT, 0, FALSE) < 0) {
      status = FTL_MALLOC_FAILURE;
      break;
    }

    if (os_semaphore_create(&ftl->audio.media_component.pkt_ready, "/AudioPkt", O_CREAT, 0, FALSE) < 0) {
        status = FTL_MALLOC_FAILURE;
        break;
    }

    // We need set this flag now so it is ready when the thread starts, but also
    // so it is set if we destroy this before the thread starts it will be cleaned up.
    ftl_set_state(ftl, FTL_TX_THRD);
    if ((os_create_thread(&media->video_send_thread, NULL, video_send_thread, ftl)) != 0) {
      ftl_clear_state(ftl, FTL_TX_THRD);
      status = FTL_MALLOC_FAILURE;
      break;
    }

    // We need set this flag now so it is ready when the thread starts, but also
    // so it is set if we destroy this before the thread starts it will be cleaned up.
    ftl_set_state(ftl, FTL_TX_THRD);
    if ((os_create_thread(&media->audio_send_thread, NULL, audio_send_thread, ftl)) != 0) {
        ftl_clear_state(ftl, FTL_TX_THRD);
        status = FTL_MALLOC_FAILURE;
        break;
    }

    if (os_semaphore_create(&media->ping_thread_shutdown, "/PingThreadShutdown", O_CREAT, 0, FALSE) < 0) {
      status = FTL_MALLOC_FAILURE;
      break;
    }

    // We need set this flag now so it is ready when the thread starts, but also
    // so it is set if we destroy this before the thread starts it will be cleaned up.
    ftl_set_state(ftl, FTL_PING_THRD);
    if ((os_create_thread(&media->ping_thread, NULL, ping_thread, ftl)) != 0) {
      ftl_clear_state(ftl, FTL_PING_THRD);
      status = FTL_MALLOC_FAILURE;
      break;
    }

    ftl_clear_state(ftl, FTL_SPEED_TEST);

    ftl_set_state(ftl, FTL_MEDIA_READY);

    return FTL_SUCCESS;
  } while (0);
cleanup:

  _internal_media_destroy(ftl);

  return status;
}

int media_enable_nack(ftl_stream_configuration_private_t *ftl, uint32_t ssrc, BOOL enabled) {
  ftl_media_component_common_t *mc = NULL;
  if ((mc = _media_lookup(ftl, ssrc)) == NULL) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "Unable to find ssrc %d\n", ssrc);
    return -1;
  }

  mc->nack_enabled = enabled;

  return 0;
}

ftl_status_t _internal_media_destroy(ftl_stream_configuration_private_t *ftl) {
  ftl_media_config_t *media = &ftl->media;
  ftl_status_t status = FTL_SUCCESS;

  // Close while socket still active
  if (ftl_get_state(ftl, FTL_PING_THRD)) {
    ftl_clear_state(ftl, FTL_PING_THRD);
    os_semaphore_post(&media->ping_thread_shutdown);
    os_wait_thread(media->ping_thread);
    os_destroy_thread(media->ping_thread);
    os_semaphore_delete(&media->ping_thread_shutdown);
  }

  // Close while socket still active
  if (ftl_get_state(ftl, FTL_TX_THRD)) {
    ftl_clear_state(ftl, FTL_TX_THRD);
    os_semaphore_post(&ftl->video.media_component.pkt_ready);
    os_semaphore_post(&ftl->audio.media_component.pkt_ready);
    os_wait_thread(media->video_send_thread);
    os_wait_thread(media->audio_send_thread);
    os_destroy_thread(media->video_send_thread);
    os_destroy_thread(media->audio_send_thread);
    os_semaphore_delete(&ftl->video.media_component.pkt_ready);
    os_semaphore_delete(&ftl->audio.media_component.pkt_ready);
  }

  // Stop the receive thread while the socket is open.
  if (ftl_get_state(ftl, FTL_RX_THRD)) {
    ftl_clear_state(ftl, FTL_RX_THRD);
    os_wait_thread(media->recv_thread);
    os_destroy_thread(media->recv_thread);
  }

  // Shutdown the socket
  {
    os_lock_mutex(&media->mutex);
    if (media->media_socket != INVALID_SOCKET) {
      shutdown_socket(media->media_socket, SD_BOTH);
      close_socket(media->media_socket);
      media->media_socket = INVALID_SOCKET;
      if (media->ingest_addr) {
        free(media->ingest_addr);
      }
    }
    os_unlock_mutex(&media->mutex);
  }

  ftl_media_component_common_t *video_comp = &ftl->video.media_component;
  _nack_destroy(video_comp);

  ftl_media_component_common_t *audio_comp = &ftl->audio.media_component;
  _nack_destroy(audio_comp);

  media->max_mtu = 0;
  os_delete_mutex(&media->mutex);
  os_delete_mutex(&ftl->audio.mutex);
  os_delete_mutex(&ftl->video.mutex);

  return status;
}

ftl_status_t media_destroy(ftl_stream_configuration_private_t *ftl) {

  ftl_status_t ret = FTL_SUCCESS;

  if (!ftl_get_state(ftl, FTL_MEDIA_READY)) {
    return ret;
  }

  // Take the locks and then clear the flag.
  // This will ensure we aren't in the middle of a send
  // and prevent data from being sent.
  os_lock_mutex(&ftl->audio.mutex);
  os_lock_mutex(&ftl->video.mutex);

  ftl_clear_state(ftl, FTL_MEDIA_READY);

  os_unlock_mutex(&ftl->video.mutex);
  os_unlock_mutex(&ftl->audio.mutex);

  while (ftl_get_state(ftl, FTL_SPEED_TEST)) {
    sleep_ms(250);
  }

  // Note this will delete the mutexes used above.
  ret = _internal_media_destroy(ftl);

  return ret;
}

static int _nack_init(ftl_media_component_common_t *media) {

  int i;
  for (i = 0; i < NACK_RB_SIZE; i++) {
    if ((media->nack_slots[i] = (nack_slot_t *)malloc(sizeof(nack_slot_t))) == NULL) {
      return FTL_MALLOC_FAILURE;
    }

    nack_slot_t *slot = media->nack_slots[i];

    os_init_mutex(&slot->mutex);

    slot->len = 0;
    slot->sn = -1;
    slot->isPartOfIframe = 0;
  }

  os_init_mutex(&media->nack_slots_lock);
  media->nack_slots_initalized = TRUE;
  media->nack_enabled = TRUE;
  media->seq_num = media->xmit_seq_num = 0; //TODO: should start at a random value

  return FTL_SUCCESS;
}

static int _nack_destroy(ftl_media_component_common_t *media) {
  int i;
  for (i = 0; i < NACK_RB_SIZE; i++) {
    if (media->nack_slots[i] != NULL) {
      os_delete_mutex(&media->nack_slots[i]->mutex);
      free(media->nack_slots[i]);
      media->nack_slots[i] = NULL;
    }
  }
  os_delete_mutex(&media->nack_slots_lock);
  return 0;
}

void _clear_stats(media_stats_t *stats) {
  stats->frames_received = 0;
  stats->frames_sent = 0;
  stats->bw_throttling_count = 0;
  stats->bytes_sent = 0;
  stats->payload_bytes_sent = 0;
  stats->packets_sent = 0;
  stats->late_packets = 0;
  stats->lost_packets = 0;
  stats->nack_requests = 0;
  stats->dropped_frames = 0;
  stats->bytes_queued = 0;
  stats->packets_queued = 0;
  stats->pkt_xmit_delay_max = 0;
  stats->pkt_xmit_delay_min = 10000;
  stats->total_xmit_delay = 0;
  stats->xmit_delay_samples = 0;
  stats->pkt_rtt_max = 0;
  stats->pkt_rtt_min = 10000;
  stats->total_rtt = 0;
  stats->rtt_samples = 0;
  stats->current_frame_size = 0;
  stats->max_frame_size = 0;
  gettimeofday(&stats->start_time, NULL);
}

void _update_timestamp(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc, int64_t dts_usec) {

  // If we don't have a ntp base time set grab it now.
  if (ftl->media.sender_report_base_ntp.tv_sec == 0 &&
    ftl->media.sender_report_base_ntp.tv_usec == 0)
  {
    gettimeofday(&ftl->media.sender_report_base_ntp, NULL);
    FTL_LOG(ftl, FTL_LOG_INFO, "Sender report base ntp time set to %llu us\n", mc->payload_type, timeval_to_us(&ftl->media.sender_report_base_ntp));
  }

  if (mc->base_dts_usec < 0) {
    mc->base_dts_usec = dts_usec;
    FTL_LOG(ftl, FTL_LOG_INFO, "Stream (%lu) base dts set to %llu \n", mc->payload_type, dts_usec);
  }

  // Convert the incoming dts time to the correct clock time for the timestamp.
  // We use a int64 to ensure the roll over is handled correctly.
  // We do the [USEC_IN_SEC / 2] trick to make sure the result of the division rounds to the nearest int.
  uint64_t timestamp = ((dts_usec - mc->base_dts_usec) * ((uint64_t)(mc->timestamp_clock)));
  mc->timestamp = (uint32_t)((timestamp + USEC_IN_SEC / 2) / USEC_IN_SEC);
  mc->timestamp_dts_usec = dts_usec;
}

ftl_status_t media_speed_test(ftl_stream_configuration_private_t *ftl, int speed_kbps, int duration_ms, speed_test_t *results) {
  ftl_media_component_common_t *mc = &ftl->audio.media_component;
  ftl_media_config_t *media = &ftl->media;
  int64_t bytes_sent = 0;
  int error = 0;
  int effective_kbps = -1;
  ftl_status_t retval = FTL_SPEED_TEST_ABORTED;
  int64_t transmit_level = MAX_MTU;
  unsigned char data[MAX_MTU];
  int bytes_per_ms;
  int64_t total_ms = 0;
  struct timeval stop_tv, start_tv, delta_tv, sendToTimeLoopTime_tv;
  float packet_loss = 0.f;
  int64_t ms_elapsed;
  int64_t total_sent = 0;
  int64_t pkts_sent = 0;
  ping_pkt_t *ping;
  nack_slot_t slot;
  uint8_t fmt = 1; //generic nack
  uint8_t ptype = PING_PTYPE;
  int wait_retries;
  int initial_rtt, final_rtt;

  ftl_set_state(ftl, FTL_SPEED_TEST);

  if (!ftl_get_state(ftl, FTL_MEDIA_READY)) {
    ftl_clear_state(ftl, FTL_SPEED_TEST);
    return retval;
  }

  media_enable_nack(ftl, mc->ssrc, FALSE);
  ftl_set_state(ftl, FTL_DISABLE_TX_PING_PKTS);
  ftl->video.has_sent_first_frame = TRUE;

  ping = (ping_pkt_t*)slot.packet;

  int rtp_hdr_len = 0;
  slot.len = sizeof(ping_pkt_t);

  ping->header = htonl((2 << 30) | (fmt << 24) | (ptype << 16) | sizeof(ping_pkt_t));

  // Send ping packet first to get an accurate estimate of rtt under ideal conditions.
  // We send it multiplies times to try to ensure one makes it on poor connections.
  ftl->media.last_rtt_delay = -1;
  gettimeofday(&ping->xmit_time, NULL);
  _media_send_slot(ftl, &slot);
  _media_send_slot(ftl, &slot);
  _media_send_slot(ftl, &slot);

  wait_retries = 5;
  while ((initial_rtt = ftl->media.last_rtt_delay) < 0 && wait_retries-- > 0) {
    sleep_ms(25);
  };

  results->starting_rtt = (wait_retries <= 0) ? -1 : ftl->media.last_rtt_delay;

  int64_t initial_nack_cnt = mc->stats.nack_requests;

  gettimeofday(&start_tv, NULL);

  bytes_per_ms = speed_kbps * 1000 / 8 / 1000;

  while (total_ms < duration_ms && !error) {

    if (transmit_level <= 0) {
      sleep_ms(MAX_MTU / bytes_per_ms + 1);
    }

    gettimeofday(&stop_tv, NULL);
    timeval_subtract(&delta_tv, &stop_tv, &start_tv);
    ms_elapsed = (int64_t)timeval_to_ms(&delta_tv);
    transmit_level += ms_elapsed * bytes_per_ms;
    total_ms += ms_elapsed;

    start_tv = stop_tv;

    while (transmit_level > 0) {
      pkts_sent++;
      if ((bytes_sent = media_send_audio(ftl, 0, data, sizeof(data))) < sizeof(data)) {
        error = 1;
        break;
      }

      // Sendto (which is called in media_send_audio) can block if the computer's network connection is bad
      // and the local OS send buffer is full. We want this behavior when streaming normally for the send thread
      // to throttle the amount of data we send, but during the speed test this causes us to block the send loop and makes
      // the speed test too long and return inaccurate values.
      gettimeofday(&sendToTimeLoopTime_tv, NULL);
      timeval_subtract(&delta_tv, &sendToTimeLoopTime_tv, &start_tv);
      int64_t ms_timeSinceLoopStart = (int64_t)timeval_to_ms(&delta_tv);
      if (ms_timeSinceLoopStart + ms_elapsed > duration_ms)
      {
        total_ms = duration_ms;
        break;
      }

      total_sent += bytes_sent;
      transmit_level -= bytes_sent;
    }
  }

  if (!error) {

    // After the test send another ping packet to detect rtt.
    // We might need to send a few of these to make sure one makes it
    // after we burst the network with packets in the test.
    ftl->media.last_rtt_delay = -1;
    wait_retries = 2000 / PING_TX_INTERVAL_MS; // waiting up to 2s for ping to come back
    while (ftl->media.last_rtt_delay < 0 && wait_retries-- > 0)
    {
      // Send the ping packet
      gettimeofday(&ping->xmit_time, NULL);
      _media_send_slot(ftl, &slot);

      // Sleep for a bit.
      sleep_ms(PING_TX_INTERVAL_MS);
    }

    final_rtt = ftl->media.last_rtt_delay;
    results->ending_rtt = (wait_retries <= 0) ? -1 : ftl->media.last_rtt_delay;

    //if we lost a ping packet ignore rtt, if the final rtt is lower than the initial ignore
    if (initial_rtt < 0 || final_rtt < 0 || final_rtt < initial_rtt) {
      initial_rtt = final_rtt = 0;
    }

    //if we didnt get the last ping packet assume the worst for rtt
    if (wait_retries <= 0) {
      initial_rtt = 0;
      final_rtt = 2000;
    }

    int64_t lost_pkts = mc->stats.nack_requests - initial_nack_cnt;
    float pkt_loss_percent = (float)lost_pkts / (float)pkts_sent;

    float adjusted_bytes_sent = (float)total_sent * (1.f - pkt_loss_percent);
    int64_t actual_send_time = total_ms + final_rtt - initial_rtt;
    effective_kbps = (int)(((float)adjusted_bytes_sent * 8.f * 1000.f / (float)actual_send_time) / 1000.f);

    results->pkts_sent = (int)pkts_sent;
    results->nack_requests = (int)lost_pkts;
    results->lost_pkts = (int)lost_pkts;
    results->bytes_sent = (int)total_sent;
    results->duration_ms = (int)actual_send_time;
    results->peak_kbps = effective_kbps;

    FTL_LOG(ftl, FTL_LOG_ERROR, "Sent %d bytes in %d ms; send packets %d lost %d packets; (first rtt: %d, last %d). Estimated peak bitrate %d kbps\n",
      results->bytes_sent, results->duration_ms, results->pkts_sent, results->lost_pkts, initial_rtt, final_rtt, results->peak_kbps);

    retval = FTL_SUCCESS;
  }

  // Reset all vars that were effected by the test.
  mc->seq_num = 0;
  mc->xmit_seq_num = 0;
  mc->timestamp = 0;
  mc->producer = 0;
  mc->consumer = 0;
  mc->base_dts_usec = -1;
  _clear_stats(&mc->stats);
  ftl->media.sender_report_base_ntp.tv_sec = 0;
  ftl->media.sender_report_base_ntp.tv_usec = 0;

  ftl->video.has_sent_first_frame = FALSE;
  media_enable_nack(ftl, mc->ssrc, TRUE);
  ftl_clear_state(ftl, FTL_DISABLE_TX_PING_PKTS);

  ftl_clear_state(ftl, FTL_SPEED_TEST);

  return retval;
}

int media_send_audio(ftl_stream_configuration_private_t *ftl, int64_t dts_usec, uint8_t *data, int32_t len) {
  ftl_media_component_common_t *mc = &ftl->audio.media_component;
  uint8_t nalu_type = 0;
  int bytes_sent = 0;

  int pkt_len;
  int payload_size;
  nack_slot_t *slot;
  int remaining = len;
  int retries = 0;


  // When we get our first audio packet, indicate that we are ready to send.
  // However, don't send audio data until the video is also sending.
  ftl->audio.is_ready_to_send = TRUE;
  if (!ftl->video.has_sent_first_frame)
  {
    return 0;
  }

  if (os_trylock_mutex(&ftl->audio.mutex)) {

    if (ftl_get_state(ftl, FTL_MEDIA_READY)) {

      _update_timestamp(ftl, mc, dts_usec);

      while (remaining > 0) {
        uint16_t sn = mc->seq_num;
        uint32_t ssrc = mc->ssrc;
        uint8_t *pkt_buf;

        if ((slot = _media_get_empty_slot(ftl, ssrc, sn)) == NULL) {
          return 0;
        }

        pkt_buf = slot->packet;
        pkt_len = sizeof(slot->packet);

        os_lock_mutex(&slot->mutex);

        payload_size = _media_make_audio_rtp_packet(ftl, data, remaining, pkt_buf, &pkt_len);

        remaining -= payload_size;
        data += payload_size;
        bytes_sent += pkt_len;
        mc->stats.payload_bytes_sent += payload_size;

        slot->len = pkt_len;
        slot->sn = sn;
        slot->last = 1;
        gettimeofday(&slot->insert_time, NULL);

        os_unlock_mutex(&slot->mutex);

        os_semaphore_post(&mc->pkt_ready);
      }
    }

    os_unlock_mutex(&ftl->audio.mutex);
  }

  return bytes_sent;
}

int media_send_video(ftl_stream_configuration_private_t *ftl, int64_t dts_usec, uint8_t *data, int32_t len, int end_of_frame) {
  ftl_media_component_common_t *mc = &ftl->video.media_component;
  uint8_t nalu_type = 0;
  uint8_t nri;
  int bytes_queued = 0;
  int pkt_len;
  int payload_size;
  nack_slot_t *slot;
  int remaining = len;
  int first_fu = 1;

  // Before we send any video we want to make sure the audio stream
  // is also ready to run. If the stream isn't ready drop this data.
  if (!ftl->audio.is_ready_to_send)
  {
    if (end_of_frame)
    {
      mc->stats.dropped_frames++;
    }
    return bytes_queued;
  }

  if (os_trylock_mutex(&ftl->video.mutex)) {

    if (ftl_get_state(ftl, FTL_MEDIA_READY)) {

      nalu_type = data[0] & 0x1F;
      nri = (data[0] >> 5) & 0x3;

      if (ftl->video.wait_for_idr_frame) {
        if (nalu_type == H264_NALU_TYPE_SPS) {

          ftl->video.wait_for_idr_frame = FALSE;

          if (!ftl->video.has_sent_first_frame) {
            FTL_LOG(ftl, FTL_LOG_INFO, "Audio is ready and we have the first iframe, starting stream. (dropped %d frames)\n", mc->stats.dropped_frames);
            ftl->video.has_sent_first_frame = TRUE;
          }
          else {
            FTL_LOG(ftl, FTL_LOG_INFO, "Got key frame, continuing (dropped %d frames)\n", mc->stats.dropped_frames);
          }
        }
        else {
          if (end_of_frame) {
            mc->stats.dropped_frames++;
          }
          os_unlock_mutex(&ftl->video.mutex);
          return bytes_queued;
        }
      }

      _update_timestamp(ftl, mc, dts_usec);

      if (nalu_type == H264_NALU_TYPE_IDR) {
        mc->tmp_seq_num = mc->seq_num;
      }

      while (remaining > 0) {
        uint16_t sn = mc->seq_num;
        uint32_t ssrc = mc->ssrc;
        uint8_t *pkt_buf;

        if ((slot = _media_get_empty_slot(ftl, ssrc, sn)) == NULL) {
          if (nri) {
            FTL_LOG(ftl, FTL_LOG_INFO, "Video queue full, dropping packets until next key frame\n");
            ftl->video.wait_for_idr_frame = TRUE;
          }
          os_unlock_mutex(&ftl->video.mutex);
          return bytes_queued;
        }       

        os_lock_mutex(&slot->mutex);

        pkt_buf = slot->packet;
        pkt_len = sizeof(slot->packet);

        slot->first = 0;
        slot->last = 0;

        payload_size = _media_make_video_rtp_packet(ftl, data, remaining, pkt_buf, &pkt_len, first_fu);

        first_fu = 0;
        remaining -= payload_size;
        data += payload_size;
        bytes_queued += pkt_len;
        mc->stats.payload_bytes_sent += payload_size;

        /*if all data has been consumed set marker bit*/
        if (remaining <= 0 && end_of_frame) {
          _media_set_marker_bit(mc, pkt_buf);
          slot->last = 1;
        }

        slot->len = pkt_len;
        slot->sn = sn;
        gettimeofday(&slot->insert_time, NULL);
        slot->isPartOfIframe = nalu_type == H264_NALU_TYPE_IDR;

        os_unlock_mutex(&slot->mutex);
        os_semaphore_post(&mc->pkt_ready);

        mc->stats.packets_queued++;
        mc->stats.bytes_queued += pkt_len;
      }

      mc->stats.current_frame_size += len;

      if (end_of_frame) {
        mc->stats.frames_received++;

        if (mc->stats.current_frame_size > mc->stats.max_frame_size) {
          mc->stats.max_frame_size = mc->stats.current_frame_size;
        }

        mc->stats.current_frame_size = 0;
      }
    }

    os_unlock_mutex(&ftl->video.mutex);
  }

  return bytes_queued;
}

static ftl_media_component_common_t *_media_lookup(ftl_stream_configuration_private_t *ftl, uint32_t ssrc) {
  ftl_media_component_common_t *mc = NULL;

  /*check audio*/
  mc = &ftl->audio.media_component;
  if (mc->ssrc == ssrc) {
    return mc;
  }

  /*check video*/
  mc = &ftl->video.media_component;
  if (mc->ssrc == ssrc) {
    return mc;
  }

  return NULL;
}

static nack_slot_t* _media_get_empty_slot(ftl_stream_configuration_private_t *ftl, uint32_t ssrc, uint16_t sn) {
  ftl_media_component_common_t *mc;

  if ((mc = _media_lookup(ftl, ssrc)) == NULL) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "Unable to find ssrc %d\n", ssrc);
    return NULL;
  }

  nack_slot_t *slot;
  {
    os_lock_mutex(&mc->nack_slots_lock);

    // If the next sequence number is equal to the current send number
    // the queue is full. Return null.
    // Note we do the nextSn increment outside of the if to ensure the rollover
    // for uint16 works correctly.
    uint16_t nextSn = sn + (uint16_t)1;
    if (((nextSn) % NACK_RB_SIZE) == (mc->xmit_seq_num % NACK_RB_SIZE)) {
      slot = NULL;
    }
    else {
      slot = mc->nack_slots[sn % NACK_RB_SIZE];
      slot->sn = sn;
    }

    os_unlock_mutex(&mc->nack_slots_lock);
  }

  return slot;
}

static float _media_get_queue_fullness(ftl_stream_configuration_private_t *ftl, uint32_t ssrc) {
  ftl_media_component_common_t *mc;

  if ((mc = _media_lookup(ftl, ssrc)) == NULL) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "Unable to find ssrc %d\n", ssrc);
    return -1;
  }

  int packets_queued;

  if (mc->seq_num >= mc->xmit_seq_num) {
    packets_queued = mc->seq_num - mc->xmit_seq_num;
  }
  else {
    packets_queued = 65535 - mc->xmit_seq_num + mc->seq_num + 1;
  }

  return (float)packets_queued / (float)NACK_RB_SIZE;
}

static int _media_send_slot(ftl_stream_configuration_private_t *ftl, nack_slot_t *slot) {
  int tx_len;

  uint8_t pkt[MAX_PACKET_BUFFER];
  int pkt_len;

  os_lock_mutex(&ftl->media.mutex);
  memcpy(pkt, slot->packet, slot->len);
  pkt_len = slot->len;
  os_unlock_mutex(&ftl->media.mutex);

  if ((tx_len = sendto(ftl->media.media_socket, pkt, pkt_len, 0, (struct sockaddr*) ftl->media.ingest_addr, (int)ftl->media.ingest_addrlen)) == SOCKET_ERROR)
  {
    FTL_LOG(ftl, FTL_LOG_ERROR, "sendto() failed with error: %s", get_socket_error());
  }
  
  return tx_len;
}

static int _media_send_packet(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc) {

  int tx_len;

  nack_slot_t *slot;

  {
    os_lock_mutex(&mc->nack_slots_lock);

    slot = mc->nack_slots[mc->xmit_seq_num % NACK_RB_SIZE];
    mc->xmit_seq_num++;

    os_unlock_mutex(&mc->nack_slots_lock);
  }

  os_lock_mutex(&slot->mutex);

  tx_len = _media_send_slot(ftl, slot);

  gettimeofday(&slot->xmit_time, NULL);

  if (slot->last) {
    mc->stats.frames_sent++;
  }
  mc->stats.packets_sent++;
  mc->stats.bytes_sent += tx_len;

  struct timeval profile_delta;
  float xmit_delay_delta;
  timeval_subtract(&profile_delta, &slot->xmit_time, &slot->insert_time);

  xmit_delay_delta = timeval_to_ms(&profile_delta);

  if (xmit_delay_delta > mc->stats.pkt_xmit_delay_max) {
    mc->stats.pkt_xmit_delay_max = (int)xmit_delay_delta;
  }
  else if (xmit_delay_delta < mc->stats.pkt_xmit_delay_min) {
    mc->stats.pkt_xmit_delay_min = (int)xmit_delay_delta;
  }

  mc->stats.total_xmit_delay += (int)xmit_delay_delta;
  mc->stats.xmit_delay_samples++;

  os_unlock_mutex(&slot->mutex);

  return tx_len;
}

static int _nack_resend_packet(ftl_stream_configuration_private_t *ftl, uint32_t ssrc, uint16_t sn) {
  ftl_media_component_common_t *mc;
  int tx_len = 0;

  if ((mc = _media_lookup(ftl, ssrc)) == NULL) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "Unable to find ssrc %d\n", ssrc);
    return -1;
  }

  /*map sequence number to slot*/
  nack_slot_t *slot = mc->nack_slots[sn % NACK_RB_SIZE];
  os_lock_mutex(&slot->mutex);

  if (slot->sn != sn) {
    FTL_LOG(ftl, FTL_LOG_WARN, "[%d] expected sn %d in slot but found %d...discarding retransmit request", ssrc, sn, slot->sn);
    os_unlock_mutex(&slot->mutex);
    return 0;
  }

  int req_delay = 0;
  struct timeval delta, now;
  gettimeofday(&now, NULL);
  timeval_subtract(&delta, &now, &slot->xmit_time);
  req_delay = (int)timeval_to_ms(&delta);

  if (mc->nack_enabled) {
    tx_len = _media_send_slot(ftl, slot);
    FTL_LOG(ftl, FTL_LOG_INFO, "[%d] resent sn %d, request delay was %d ms, was part of iframe? %d", ssrc, sn, req_delay, slot->isPartOfIframe);
  }
  mc->stats.nack_requests++;

  os_unlock_mutex(&slot->mutex);

  return tx_len;
}

static int _write_rtp_header(uint8_t *buf, size_t len, uint8_t ptype, uint16_t seq_num, uint32_t timestamp, uint32_t ssrc) {
  uint32_t rtp_header;

  if (RTP_HEADER_BASE_LEN > len) {
    return -1;
  }

  //TODO need to worry about alignment on some platforms
  uint32_t *out_header = (uint32_t *)buf;

  rtp_header = htonl((2 << 30) | (ptype << 16) | seq_num);

  *out_header++ = rtp_header;
  rtp_header = htonl((uint32_t)timestamp);
  *out_header++ = rtp_header;
  rtp_header = htonl(ssrc);
  *out_header++ = rtp_header;

  return (int)((uint8_t*)out_header - buf);
}

static int _media_make_video_rtp_packet(ftl_stream_configuration_private_t *ftl, uint8_t *in, int in_len, uint8_t *out, int *out_len, int first_pkt) {
  uint8_t sbit = 0, ebit = 0;
  int frag_len;
  ftl_video_component_t *video = &ftl->video;
  ftl_media_component_common_t *mc = &video->media_component;
  int rtp_hdr_len = 0;

  if ((rtp_hdr_len = _write_rtp_header(out, *out_len, mc->payload_type, mc->seq_num, mc->timestamp, mc->ssrc)) < 0) {
    return -1;
  }

  out += rtp_hdr_len;

  mc->seq_num++;

  //if this packet can fit into a it's own packet then just use single nalu mode
  if (first_pkt && in_len <= (ftl->media.max_mtu - RTP_HEADER_BASE_LEN)) {
    frag_len = in_len;
    *out_len = frag_len + rtp_hdr_len;
    memcpy(out, in, frag_len);
  }
  else {//otherwise packetize using FU-A

    if (first_pkt) {
      sbit = 1;
      video->fua_nalu_type = in[0];
      in += 1;
      in_len--;
    }
    else if (in_len <= (ftl->media.max_mtu - RTP_HEADER_BASE_LEN - RTP_FUA_HEADER_LEN)) {
      ebit = 1;
    }

    out[0] = (video->fua_nalu_type & 0x60) | 28;
    out[1] = (sbit << 7) | (ebit << 6) | (video->fua_nalu_type & 0x1F);

    out += 2;

    frag_len = ftl->media.max_mtu - rtp_hdr_len - RTP_FUA_HEADER_LEN;

    if (frag_len > in_len) {
      frag_len = in_len;
    }

    memcpy(out, in, frag_len);

    *out_len = frag_len + RTP_HEADER_BASE_LEN + RTP_FUA_HEADER_LEN;
  }

  return frag_len + sbit;
}

static int _media_make_audio_rtp_packet(ftl_stream_configuration_private_t *ftl, uint8_t *in, int in_len, uint8_t *out, int *out_len) {
  int payload_len = in_len;

  ftl_audio_component_t *audio = &ftl->audio;
  ftl_media_component_common_t *mc = &audio->media_component;

  int rtp_hdr_len = 0;

  if ((rtp_hdr_len = _write_rtp_header(out, *out_len, mc->payload_type, mc->seq_num, mc->timestamp, mc->ssrc)) < 0) {
    return -1;
  }

  out += rtp_hdr_len;

  mc->seq_num++;

  memcpy(out, in, payload_len);

  *out_len = payload_len + RTP_HEADER_BASE_LEN;

  return in_len;
}

static int _media_set_marker_bit(ftl_media_component_common_t *mc, uint8_t *in) {
  uint32_t rtp_header;

  rtp_header = ntohl(*((uint32_t*)in));
  rtp_header |= 1 << 23; /*set marker bit*/
  *((uint32_t*)in) = htonl(rtp_header);

  return 0;
}

/*handles rtcp packets from ingest including lost packet retransmission requests (nack)*/
OS_THREAD_ROUTINE recv_thread(void *data)
{
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)data;
  ftl_media_config_t *media = &ftl->media;
  int ret;
  unsigned char *buf;
  struct sockaddr_in6 ipv6_addrinfo;
  struct sockaddr_in ipv4_addrinfo;
  struct sockaddr* addrinfo;
  socklen_t addr_len, addrinfo_len;
  char remote_ip[IPVX_ADDR_ASCII_LEN];

  if (ftl->socket_family == AF_INET) {
     addrinfo = (struct sockaddr *)&ipv4_addrinfo;
     addrinfo_len = sizeof(struct sockaddr_in);
  }
  else {
     addrinfo = (struct sockaddr *)&ipv6_addrinfo;
     addrinfo_len = sizeof(struct sockaddr_in6);
  }

#ifdef _WIN32
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
    FTL_LOG(ftl, FTL_LOG_WARN, "Failed to set recv_thread priority to THREAD_PRIORITY_TIME_CRITICAL\n");
  }
#endif

  if ((buf = (unsigned char*)malloc(MAX_PACKET_BUFFER)) == NULL) {
    FTL_LOG(ftl, FTL_LOG_ERROR, "Failed to allocate recv buffer\n");
    return (OS_THREAD_TYPE)-1;
  }

  while (ftl_get_state(ftl, FTL_RX_THRD)) {

    // Wait on the socket for data or a timeout. The timeout is how we
    // exit the thread when disconnecting.
    ret = poll_socket_for_receive(media->media_socket, 50);
    if (ret == 0)
    {
      // This is a timeout, this is perfectly fine.
      continue;
    }
    else if (ret < 0)
    {
      // We hit an error.
      FTL_LOG(ftl, FTL_LOG_INFO, "Receive thread socket error on poll");
      continue;
    }

    // We have data on the socket, read it.
    addr_len = addrinfo_len;
    ret = recvfrom(media->media_socket, buf, MAX_PACKET_BUFFER, 0, (struct sockaddr *)addrinfo, &addr_len);
    if (ret <= 0) {
      // This shouldn't be possible, we should only be here is poll above told us there was data.
      FTL_LOG(ftl, FTL_LOG_INFO, "recv from failed with %s\n", get_socket_error());
      continue;
    }

    if (_get_remote_ip(addrinfo, addr_len, remote_ip, sizeof(remote_ip)) < 0) {
      continue;
    }

    if (strcmp(remote_ip, ftl->ingest_ip) != 0)
    {
      FTL_LOG(ftl, FTL_LOG_WARN, "Discarded packet from unexpected ip: %s\n", remote_ip);
      continue;
    }

    int version, padding, feedbackType, ptype, length, ssrcSender, ssrcMedia;
    uint16_t snBase, blp, sn;
    int recv_len = ret;

    if (recv_len < 2) {
      FTL_LOG(ftl, FTL_LOG_WARN, "recv packet too small to parse, discarding\n");
      continue;
    }

    /*extract rtp header*/
    version = (buf[0] >> 6) & 0x3;
    padding = (buf[0] >> 5) & 0x1;
    feedbackType = buf[0] & 0x1F;
    ptype = buf[1];

    if (feedbackType == 1 && ptype == 205) {

      length = ntohs(*((uint16_t*)(buf + 2)));

      if (recv_len < ((length + 1) * 4)) {
        FTL_LOG(ftl, FTL_LOG_WARN, "reported len was %d but packet is only %d...discarding\n", recv_len, ((length + 1) * 4));
        continue;
      }

      ssrcSender = ntohl(*((uint32_t*)(buf + 4)));
      ssrcMedia = ntohl(*((uint32_t*)(buf + 8)));

      uint16_t *p = (uint16_t *)(buf + 12);

      int fci;
      for (fci = 0; fci < (length - 2); fci++) {
        //request the first sequence number
        snBase = ntohs(*p++);
        _nack_resend_packet(ftl, ssrcMedia, snBase);
        blp = ntohs(*p++);
        if (blp) {
          int i;
          for (i = 0; i < 16; i++) {
            if ((blp & (1 << i)) != 0) {
              sn = snBase + i + 1;
              _nack_resend_packet(ftl, ssrcMedia, sn);
            }
          }
        }
      }
    }
    else if (feedbackType == 1 && ptype == PING_PTYPE) {

      ping_pkt_t *ping = (ping_pkt_t *)buf;

      struct timeval now;
      int delay_ms;
      media_stats_t *pkt_stats = &ftl->video.media_component.stats;

      gettimeofday(&now, NULL);
      delay_ms = (int)timeval_subtract_to_ms(&now, &ping->xmit_time);

      if (delay_ms > pkt_stats->pkt_rtt_max) {
        pkt_stats->pkt_rtt_max = delay_ms;
      }
      else if (delay_ms < pkt_stats->pkt_rtt_min) {
        pkt_stats->pkt_rtt_min = delay_ms;
      }

      pkt_stats->total_rtt += delay_ms;
      pkt_stats->rtt_samples++;

      ftl->media.last_rtt_delay = delay_ms;
    }
  }

  free(buf);

  FTL_LOG(ftl, FTL_LOG_INFO, "Exited Recv Thread\n");

  return (OS_THREAD_TYPE)0;
}

OS_THREAD_ROUTINE video_send_thread(void *data)
{
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)data;
  ftl_media_config_t *media = &ftl->media;
  ftl_media_component_common_t *video = &ftl->video.media_component;

  int first_packet = 1;
  int bytes_per_ms;
  int pkt_sent;
  int video_kbps = -1;
  int disable_flow_control = 1;
  int initial_peak_kbps;

  int transmit_level;
  struct timeval start_tv;

#ifdef _WIN32
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
    FTL_LOG(ftl, FTL_LOG_WARN, "Failed to set recv_thread priority to THREAD_PRIORITY_TIME_CRITICAL\n");
  }
#endif

  initial_peak_kbps = video->kbps = video->peak_kbps;
  video_kbps = 0;
  transmit_level = 5 * video->kbps * 1000 / 8 / 1000; /*small initial level to prevent bursting at the start of a stream*/

  while (1) {

    if (initial_peak_kbps != video->peak_kbps) {
      initial_peak_kbps = video->kbps = video->peak_kbps;
    }

    if (video->kbps != video_kbps) {
      bytes_per_ms = video->kbps * 1000 / 8 / 1000;
      video_kbps = video->kbps;

      disable_flow_control = 0;
      if (video_kbps <= 0) {
        disable_flow_control = 1;
      }
    }

    os_semaphore_pend(&video->pkt_ready, FOREVER);

    if (!ftl_get_state(ftl, FTL_TX_THRD)) {
      break;
    }

    if (disable_flow_control) {
      _media_send_packet(ftl, video);
    }
    else {
      pkt_sent = 0;
      if (first_packet) {
        gettimeofday(&start_tv, NULL);
        first_packet = 0;
      }

      _update_xmit_level(ftl, &transmit_level, &start_tv, bytes_per_ms);
      while (!pkt_sent && ftl_get_state(ftl, FTL_TX_THRD)) {

        if (transmit_level <= 0) {
          ftl->video.media_component.stats.bw_throttling_count++;
          sleep_ms(MAX_MTU / bytes_per_ms + 1);
          _update_xmit_level(ftl, &transmit_level, &start_tv, bytes_per_ms);
        }

        if (transmit_level > 0) {
          transmit_level -= _media_send_packet(ftl, video);
          pkt_sent = 1;
        }
      }

    }

    _update_stats(ftl);
  }

  FTL_LOG(ftl, FTL_LOG_INFO, "Exited Send Thread\n");
  return (OS_THREAD_TYPE)0;
}

OS_THREAD_ROUTINE audio_send_thread(void *data)
{
    ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)data;
    ftl_media_component_common_t *audio = &ftl->audio.media_component;

#ifdef _WIN32
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
        FTL_LOG(ftl, FTL_LOG_WARN, "Failed to set recv_thread priority to THREAD_PRIORITY_TIME_CRITICAL\n");
    }
#endif

    while (1) {

        os_semaphore_pend(&audio->pkt_ready, FOREVER);

        if (!ftl_get_state(ftl, FTL_TX_THRD)) {
            break;
        }

        _media_send_packet(ftl, audio);
    }

    FTL_LOG(ftl, FTL_LOG_INFO, "Exited Audio Send Thread\n");
    return (OS_THREAD_TYPE)0;
}

static void _update_xmit_level(ftl_stream_configuration_private_t *ftl, int *transmit_level, struct timeval *start_tv, int bytes_per_ms) {

  struct timeval stop_tv;

  gettimeofday(&stop_tv, NULL);

  *transmit_level += (int)timeval_subtract_to_ms(&stop_tv, start_tv) * bytes_per_ms;

  if (*transmit_level > (MAX_XMIT_LEVEL_IN_MS * bytes_per_ms)) {
    *transmit_level = MAX_XMIT_LEVEL_IN_MS * bytes_per_ms;
  }

  *start_tv = stop_tv;
}

static int _update_stats(ftl_stream_configuration_private_t *ftl) {
  struct timeval now;
  gettimeofday(&now, NULL);
  int stats_interval = (int)timeval_subtract_to_ms(&now, &ftl->media.stats_tv);

  if (stats_interval > 5000) {

    ftl->media.stats_tv = now;

    _send_pkt_stats(ftl, &ftl->video.media_component, stats_interval);
    _send_instant_pkt_stats(ftl, &ftl->video.media_component, stats_interval);
    _send_video_stats(ftl, &ftl->video.media_component, stats_interval);
  }

  return 0;
}

static int _send_pkt_stats(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc, int interval_ms) {
  ftl_status_msg_t m;
  m.type = FTL_STATUS_VIDEO_PACKETS;
  ftl_packet_stats_msg_t *p = &m.msg.pkt_stats;
  struct timeval now;

  gettimeofday(&now, NULL);
  p->period = timeval_subtract_to_ms(&now, &mc->stats.start_time);
  p->sent = mc->stats.packets_sent;
  p->nack_reqs = mc->stats.nack_requests;
  p->lost = 0; // needs rtcp reports to get this value
  p->recovered = 0; // need rtcp reports to get this value
  p->late = 0; // need rtcp reports to get this value

  enqueue_status_msg(ftl, &m);

  return 0;
}

static int _send_instant_pkt_stats(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc, int interval_ms) {
  ftl_status_msg_t m;
  m.type = FTL_STATUS_VIDEO_PACKETS_INSTANT;
  ftl_packet_stats_instant_msg_t *p = &m.msg.ipkt_stats;

  p->period = (int)interval_ms;
  p->min_rtt = mc->stats.pkt_rtt_min;
  p->max_rtt = mc->stats.pkt_rtt_max;
  p->avg_rtt = (mc->stats.rtt_samples) ? mc->stats.total_rtt / mc->stats.rtt_samples : 0;
  p->min_xmit_delay = mc->stats.pkt_xmit_delay_min;
  p->max_xmit_delay = mc->stats.pkt_xmit_delay_max;
  p->avg_xmit_delay = (mc->stats.xmit_delay_samples) ? mc->stats.total_xmit_delay / mc->stats.xmit_delay_samples : 0;

  mc->stats.pkt_xmit_delay_max = 0;
  mc->stats.pkt_xmit_delay_min = 10000;
  mc->stats.total_xmit_delay = 0;
  mc->stats.xmit_delay_samples = 0;

  // rtt stats are cleared by adaptive bitrate thread.
  mc->stats.pkt_rtt_max = 0;
  mc->stats.pkt_rtt_min = 10000;
  //mc->stats.total_rtt = 0;
  //mc->stats.rtt_samples = 0;

  enqueue_status_msg(ftl, &m);

  return 0;
}

static int _send_video_stats(ftl_stream_configuration_private_t *ftl, ftl_media_component_common_t *mc, int interval_ms) {
  ftl_status_msg_t m;
  ftl_video_frame_stats_msg_t *v = &m.msg.video_stats;
  struct timeval now;

  m.type = FTL_STATUS_VIDEO;

  gettimeofday(&now, NULL);
  v->period = timeval_subtract_to_ms(&now, &mc->stats.start_time);

  v->frames_queued = mc->stats.frames_received;
  v->frames_sent = mc->stats.frames_sent;
  v->bw_throttling_count = mc->stats.bw_throttling_count;
  v->bytes_queued = mc->stats.bytes_queued;
  v->bytes_sent = mc->stats.bytes_sent;
  v->queue_fullness = (int)(_media_get_queue_fullness(ftl, mc->ssrc) * 100.f);
  v->max_frame_size = mc->stats.max_frame_size;

  mc->stats.max_frame_size = 0;
  enqueue_status_msg(ftl, &m);

  return 0;
}

OS_THREAD_ROUTINE ping_thread(void *data) {

  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)data;
  ftl_media_config_t *media = &ftl->media;
  struct timeval lastSenderReportSendTime_tv;

  senderReport_pkt_t *senderReport;
  nack_slot_t senderReportSlot;
  ping_pkt_t *ping;
  nack_slot_t pingSlot;

  ping = (ping_pkt_t*)pingSlot.packet;
  senderReport = (senderReport_pkt_t*)senderReportSlot.packet;

  pingSlot.len = sizeof(ping_pkt_t);
  senderReportSlot.len = sizeof(senderReport_pkt_t);

  //   RTPC Header Format
  //      0                   1                   2                   3
  //    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  //   |V=2|P|   FMT   |       PT      |          length               |
  //   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  uint8_t fmt = 1;
  uint8_t ptype = PING_PTYPE;
  ping->header = htonl((2 << 30) | (fmt << 24) | (ptype << 16) | sizeof(ping_pkt_t));

  fmt = 0;
  ptype = SENDER_REPORT_PTYPE;
  senderReport->header = htonl((2 << 30) | (fmt << 24) | (ptype << 16) | ((sizeof(senderReport_pkt_t) / 4) - 1));

  while (ftl_get_state(ftl, FTL_PING_THRD)) {

    os_semaphore_pend(&ftl->media.ping_thread_shutdown, PING_TX_INTERVAL_MS);

    // Get the current time in ntp
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    // It's important that this is a disable check not an enable check
    // because it is possible that this flag will be set before this thread spawns.
    // In that case we don't want to overwrite the flag with the FTL_PING_THRD set above.
    if (!ftl_get_state(ftl, FTL_DISABLE_TX_PING_PKTS))
    {
        ping->xmit_time.tv_sec = currentTime.tv_sec;
        ping->xmit_time.tv_usec = currentTime.tv_usec;
        _media_send_slot(ftl, &pingSlot);
    }

    if (!ftl_get_state(ftl, FTL_DISABLE_TX_SENDER_REPORT))
    {
        uint64_t timeSinceLastSRSendMs = timeval_subtract_to_ms(&currentTime, &lastSenderReportSendTime_tv);
        if (timeSinceLastSRSendMs > SENDER_REPORT_TX_INTERVAL_MS)
        {
            lastSenderReportSendTime_tv = currentTime;

            // For each media component...
            ftl_media_component_common_t *media_comp[] = { &ftl->video.media_component, &ftl->audio.media_component };
            ftl_media_component_common_t *comp;
            int mediaCount = 0;
            for (mediaCount = 0; mediaCount < sizeof(media_comp) / sizeof(media_comp[0]); mediaCount++) {

                comp = media_comp[mediaCount];

                // Ensure the stream has been started.
                if (comp->base_dts_usec < 0)
                {
                    continue;
                }

                // Set the ssrc and packet counts
                senderReport->ssrc = htonl(comp->ssrc);
                senderReport->senderOctetCount = htonl((uint32_t)comp->stats.payload_bytes_sent);
                senderReport->senderPacketCount = htonl((uint32_t)comp->stats.packets_sent);

                // Grab the last rtp timestamp. Since this is multi threaded we need it locally to ensure it doesn't change.
                uint64_t timestamp = comp->timestamp;
                uint64_t timestamp_dts_usec = comp->timestamp_dts_usec;
                senderReport->rtpTimestamp = htonl((uint32_t)timestamp);

                // For the NTP time, we will take the base ntp time for this stream and increment it by the amount of time
                // that has passed from this rtp timestamp and the base timestamp. This way all of the values are derived from
                // the timestamps we are passed from the client. The base time of the ntp clock doesn't really matter, it can't be
                // trusted off this computer anyways due to clock sync. The ntp time is only use to relatively to compare SR reports.
                uint64_t timeDiff_usec = timestamp_dts_usec - comp->base_dts_usec;

                struct timeval srNtpTimestamp = media->sender_report_base_ntp;
                timeval_add_us(&srNtpTimestamp, timeDiff_usec);

                uint64_t ntpTimestamp = timeval_to_ntp(&srNtpTimestamp);
                senderReport->ntpTimestampHigh = htonl(ntpTimestamp >> 32);
                senderReport->ntpTimestampLow = htonl((uint32_t)(ntpTimestamp));

                // Send the report
                _media_send_slot(ftl, &senderReportSlot);
            }
        }
    }
  }

  FTL_LOG(ftl, FTL_LOG_INFO, "Exited Ping Thread\n");

  return 0;
}

// ================================================ Bitrate Monitor Logic =================================================== //

FTL_API ftl_status_t ftl_get_video_stats(
  ftl_handle_t* handle,
  uint64_t* frames_sent,
  uint64_t* nacks_received,
  uint64_t* rtt_recorded,
  uint64_t* frames_dropped,
  float* queue_fullness
)
{
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)handle->priv;
  ftl_media_component_common_t *mc = &ftl->video.media_component;
  *frames_sent = ftl->video.media_component.stats.frames_sent;
  *nacks_received = ftl->video.media_component.stats.nack_requests;
  *rtt_recorded = (mc->stats.rtt_samples) ? mc->stats.total_rtt / mc->stats.rtt_samples : 0;
  *frames_dropped = mc->stats.dropped_frames;
  *queue_fullness = _media_get_queue_fullness(ftl, mc->ssrc);

  mc->stats.pkt_rtt_max = 0;
  mc->stats.pkt_rtt_min = 10000;
  mc->stats.total_rtt = 0;
  mc->stats.rtt_samples = 0;

  return FTL_SUCCESS;
}

BOOL is_bitrate_reduction_required(
  const float nacks_to_frames_ratio,
  const float avg_rtt,
  const uint64_t avg_frames_dropped_per_second,
  const float queue_fullness)
{
  // TODO : Improve estimation of rtt stability.
  if (nacks_to_frames_ratio > MIN_NACKS_RECEIVED_TO_PACKETS_SENT_RATIO_FOR_BITRATE_DOWNGRADE
    || avg_frames_dropped_per_second > 3
    || avg_rtt > MIN_AVG_RTT_TO_DEEM_BW_CONSTRAINED
    || queue_fullness > MIN_QUEUE_FULLNESS_TO_DEEM_BW_CONSTRAINED
    )
  {
    return TRUE;
  }
  return FALSE;
}

BOOL is_bw_stable(
  const float nacks_to_frames_ratio,
  const float avg_rtt,
  const uint64_t avg_frames_dropped_per_second,
  const float queue_fullness)
{
  // TODO : Improve estimation of rtt stability
  if (nacks_to_frames_ratio < MAX_NACKS_RECEIVED_TO_PACKETS_SENT_RATIO_FORBITRATE_UPGRADE
    && avg_frames_dropped_per_second == 0
    && avg_rtt < MAX_AVG_RTT_TO_DEEM_BW_STABLE
    && queue_fullness < MAX_QUEUE_FULLNESS_TO_DEEM_BW_STABLE
    )
  {
    return TRUE;
  }
  return FALSE;
}

uint64_t compute_recommended_bitrate(
  const uint64_t current_encoding_bitrate,
  const uint64_t max_encoding_bitrate,
  const uint64_t min_encoding_bitrate,
  ftl_bitrate_changed_reason_t reason
)
{
  uint64_t recommended_bitrate = 0;

  if (reason == FTL_BANDWIDTH_CONSTRAINED)
  {
    recommended_bitrate = BW_INSUFFICIENT_BITRATE_DOWNGRADE_PERCENTAGE * current_encoding_bitrate / 100;
  }

  else if (reason == FTL_BANDWIDTH_AVAILABLE)
  {
    recommended_bitrate = current_encoding_bitrate + BW_IDEAL_BITRATE_UPGRADE_BPS;
  }
  else
  {
    recommended_bitrate = REVERT_TO_STABLE_BITRATE_DOWNGRADE_PERCENTAGE * current_encoding_bitrate / 100;
  }

  if (recommended_bitrate < min_encoding_bitrate)
  {
    recommended_bitrate = min_encoding_bitrate;
  }

  if (recommended_bitrate > max_encoding_bitrate)
  {
    recommended_bitrate = max_encoding_bitrate;
  }
  return recommended_bitrate;
}

FTL_API ftl_status_t ftl_adaptive_bitrate_thread(ftl_handle_t* ftl_handle, void* context, int(*change_bitrate_callback)(void*, uint64_t), uint64_t initial_encoding_bitrate, uint64_t min_encoding_bitrate, uint64_t max_encoding_bitrate)
{
  ftl_status_t ret_status = FTL_SUCCESS;
  ftl_stream_configuration_private_t *ftl = (ftl_stream_configuration_private_t *)ftl_handle->priv;
  ftl_adaptive_bitrate_thread_params_t* thread_params = NULL;

  do
  {
    if ((thread_params = (ftl_adaptive_bitrate_thread_params_t*)malloc(sizeof(ftl_adaptive_bitrate_thread_params_t))) == NULL)
    {
      ret_status = FTL_MALLOC_FAILURE;
      break;
    }

    memset(thread_params, 0, sizeof(thread_params));

    thread_params->handle = ftl_handle;
    thread_params->change_bitrate_callback = change_bitrate_callback;
    thread_params->initial_encoding_bitrate = initial_encoding_bitrate;
    thread_params->max_encoding_bitrate = max_encoding_bitrate;
    thread_params->min_encoding_bitrate = min_encoding_bitrate;
    thread_params->context = context;

    if (os_semaphore_create(&ftl->bitrate_thread_shutdown, "/BitrateThreadShutdown", O_CREAT, 0, FALSE) < 0)
    {
      ret_status = FTL_MALLOC_FAILURE;
      break;
    }

    if ((os_create_thread(&ftl->bitrate_monitor_thread, NULL, adaptive_bitrate_thread, thread_params)) != 0)
    {
      os_semaphore_delete(&ftl->bitrate_thread_shutdown);
      ret_status = FTL_MALLOC_FAILURE;
      break;
    }
    // Only set this after the thread has actually been created.
    ftl_set_state(ftl, FTL_BITRATE_THRD);
  } while (0);

  if (ret_status != FTL_SUCCESS)
  {
    free(thread_params);
  }
  return ret_status;
}

// The threads looks at the stats over the last 5 seconds of data to estimate bandwidth conditoins and compute whether upgrade or downgrade
// is required. If bandwidth is constrained we go down by a large amount and come back up slowly. Once upgrade is too excessive
// we revert to the previous bitrate, and do not try an upgrade for 10 mins.
OS_THREAD_ROUTINE adaptive_bitrate_thread(void* data)
{
  ftl_adaptive_bitrate_thread_params_t *params = (ftl_adaptive_bitrate_thread_params_t *)data;
  ftl_stream_configuration_private_t* ftl = (ftl_stream_configuration_private_t*)params->handle->priv;

  // We choose a duration of BW_CHECK_DURATION_MS milliseconds to estimate bandiwidth conditions. The duration is sampled at a period of 
  // STREAM_STATS_CAPTURE_MS. Hence the number of stats we save in our circular buffer is MAX_STAT_SIZE.
  assert(MAX_STAT_SIZE == BW_CHECK_DURATION_MS / STREAM_STATS_CAPTURE_MS);

  FTL_LOG(params->handle->priv, FTL_LOG_INFO, "Starting adaptive bitrate thread");

  // Circular buffers to hold bw stats data queried via ftl_get_video_params.
  // The stats data shows data aggregated over the last c_ulBwCheckDurationMs milliseconds.
  uint64_t nacks_received[MAX_STAT_SIZE] = {0};
  uint64_t frames_sent[MAX_STAT_SIZE] = {0};
  uint64_t rtts_received[MAX_STAT_SIZE] = {0};
  uint64_t frames_dropped[MAX_STAT_SIZE] = {0};
  float queue_fullness = 0; // queue fullness doesnt need to be aggregated. If it goes above a threshold value we deem bw unstable.

  uint32_t current_position_of_circular_buffer = 0;
  BOOL circular_buffer_is_full = FALSE;

  uint64_t last_frames_sent_recorded = 0;
  uint64_t last_nacks_received_recorded = 0;
  uint64_t last_frames_dropped_recorded = 0;
  uint64_t last_rtt_received = 0;

  ftl_get_video_stats(
    params->handle,
    &last_frames_sent_recorded,
    &last_nacks_received_recorded,
    &last_rtt_received,
    &last_frames_dropped_recorded,
    &queue_fullness
  );
  // Current bitrate encoding as a percentage of the original encoding bitrate.
  uint64_t current_encoding_bitrate = params->initial_encoding_bitrate;

  struct timeval last_bitrate_upgrade_time, bw_upgrade_freeze_start_time;
  gettimeofday(&last_bitrate_upgrade_time, NULL);
  bw_upgrade_freeze_start_time = (struct timeval) { 0 };

  BOOL bitrate_changed = FALSE;
  BOOL attempt_to_revert_to_stable_bandwidth_first = FALSE;

  // If a bitrate is deemed as stable after update, we report it to telemetry.
  //  bitrate is only deemed stable when we reach back to original bitrate, or when we revert to a bitrate after an excessive upgrade.
  BOOL check_bitrate_for_stability = FALSE;

  while (1)
  {
    os_semaphore_pend(&ftl->bitrate_thread_shutdown, 0);
    if (!ftl_get_state(params->handle->priv, FTL_BITRATE_THRD))
    {
      break;
    }

    uint64_t nacks_received_recorded = 0;
    uint64_t frames_sent_recorded = 0;
    uint64_t rtt_received = 0;
    uint64_t frames_dropped_recorded = 0;

    ftl_get_video_stats(params->handle, &frames_sent_recorded, &nacks_received_recorded, &rtt_received, &frames_dropped_recorded, &queue_fullness);

    uint64_t nacks_received_since_last_check = nacks_received_recorded - last_nacks_received_recorded;
    uint64_t frames_sent_since_last_check = frames_sent_recorded - last_frames_sent_recorded;
    uint64_t frames_dropped_since_last_check = frames_dropped_recorded - last_frames_dropped_recorded;

    last_nacks_received_recorded = nacks_received_recorded;
    last_frames_sent_recorded = frames_sent_recorded;
    last_frames_dropped_recorded = frames_dropped_recorded;

    nacks_received[current_position_of_circular_buffer] = nacks_received_since_last_check;
    frames_sent[current_position_of_circular_buffer] = frames_sent_since_last_check;
    rtts_received[current_position_of_circular_buffer] = rtt_received;
    frames_dropped[current_position_of_circular_buffer] = frames_dropped_since_last_check;

    // Once circular buffer is full, set the flag as we now have enough data to check bandwidth is constrained.
    if (current_position_of_circular_buffer + 1 >= MAX_STAT_SIZE)
    {
      circular_buffer_is_full = TRUE;
    }
    // Update position in circular buffer.
    current_position_of_circular_buffer = (current_position_of_circular_buffer + 1) % MAX_STAT_SIZE;

    // This conditions ensures that we have stats of the last c_ulBwCheckDurationMs milliseconds.
    if (circular_buffer_is_full)
    {
      uint64_t nacks_received_total = 0;
      uint64_t frames_sent_total = 0;
      uint64_t total_rtt = 0;
      float avg_rtt = 0;
      uint64_t frames_dropped_total = 0;
      uint64_t avg_frames_dropped_per_second = 0;
      float nacks_to_frames_ratio = 0;
      int i;

      // Count all nacks received for the last c_ulBwCheckDurationMs milliseconds
      for (i = 0; i < MAX_STAT_SIZE; i++)
      {
        nacks_received_total += nacks_received[i];
      }
      // Count all frames sent over the last c_ulBwCheckDurationMs milliseconds
      for (i = 0; i< MAX_STAT_SIZE; i++)
      {
        frames_sent_total += frames_sent[i];
      }

      if (frames_sent_total != 0)
      {
        nacks_to_frames_ratio = (float)nacks_received_total / (float)frames_sent_total;
      }

      for (i = 0; i< MAX_STAT_SIZE; i++)
      {
        total_rtt += rtts_received[i];
      }
      avg_rtt = (float)total_rtt / (float)MAX_STAT_SIZE;

      for (i = 0; i < MAX_STAT_SIZE; i++)
      {
        frames_dropped_total += frames_dropped[i];
      }
      avg_frames_dropped_per_second = (frames_dropped_total / (BW_CHECK_DURATION_MS / 1000));

      // Check if bandwidth is constrained and bitrate reduction is required. The bandwidth can be constrained for two reasons.
      // Either the available bandwidth has decreased, or we tried to upgrade the bitrate and its too excessive.
      if (is_bitrate_reduction_required(nacks_to_frames_ratio, avg_rtt, avg_frames_dropped_per_second, queue_fullness))
      {
        FTL_LOG(params->handle->priv, FTL_LOG_INFO, "Bitrate reduction required. Nacks Received %d , Frames Sent %d rtt %d queue_fullness %4.2f",
          nacks_received_total,
          frames_sent_total,
          avg_rtt,
          queue_fullness
        );

        // If we had previously upgraded the bitrate and we started seeing the 
        // constrain within c_iMaxSecondsToDeemBitrateUpgradeExcessive seconds, it means our bitrate upgrade was excessiver. 
        // we should revert back to a lower bitrate and freeze all upgrades for c_uBitrateUpgradeFreezeTimeMs milliseconds.
        if (attempt_to_revert_to_stable_bandwidth_first && get_ms_elapsed_since(&last_bitrate_upgrade_time) < MAX_MS_TO_DEEM_UPGRADE_EXCESSIVE)
        {
          FTL_LOG(params->handle->priv, FTL_LOG_INFO, "Reverting to a stable bitrate and freezing upgrade");
          uint64_t recommended_bitrate = compute_recommended_bitrate(current_encoding_bitrate, params->max_encoding_bitrate, params->min_encoding_bitrate, FTL_UPGRADE_EXCESSIVE);

          BOOL changeBitrateResult = params->change_bitrate_callback(params->context, recommended_bitrate);

          if (changeBitrateResult)
          {
            ftl_bitrate_changed_msg_t msg =
            {
              FTL_BITRATE_DECREASED,
              FTL_UPGRADE_EXCESSIVE,
              recommended_bitrate,
              current_encoding_bitrate,
              0.f,
              avg_rtt,
              avg_frames_dropped_per_second,
              queue_fullness
            };
            ftl_status_msg_t status_msg;
            status_msg.type = FTL_BITRATE_CHANGED;
            status_msg.msg.bitrate_changed_msg = msg;
            enqueue_status_msg(params->handle->priv, &status_msg);

            bitrate_changed = TRUE;
            check_bitrate_for_stability = TRUE;
            attempt_to_revert_to_stable_bandwidth_first = FALSE;
            current_encoding_bitrate = recommended_bitrate;
            gettimeofday(&bw_upgrade_freeze_start_time, NULL);
          }
        }
        // This means the available bandiwdth seems to have decreased and we need to reduce our bitrate to comply.
        else
        {
          uint64_t recommended_bitrate = compute_recommended_bitrate(current_encoding_bitrate, params->max_encoding_bitrate, params->min_encoding_bitrate, FTL_BANDWIDTH_CONSTRAINED);
          BOOL changeBitrateResult = params->change_bitrate_callback(params->context, recommended_bitrate);
          // We had to lower bitrate. Bitrate is not stable.
          check_bitrate_for_stability = FALSE;
          if (changeBitrateResult)
          {
            ftl_bitrate_changed_msg_t msg =
            {
              FTL_BITRATE_DECREASED,
              FTL_BANDWIDTH_CONSTRAINED,
              recommended_bitrate,
              current_encoding_bitrate,
              nacks_to_frames_ratio,
              avg_rtt,
              avg_frames_dropped_per_second,
              queue_fullness
            };
            ftl_status_msg_t status_msg;
            status_msg.type = FTL_BITRATE_CHANGED;
            status_msg.msg.bitrate_changed_msg = msg;
            enqueue_status_msg(params->handle->priv, &status_msg);

            bitrate_changed = TRUE;
            current_encoding_bitrate = recommended_bitrate;
          }
        }
      }
      // If bandwidth is stable and we are haven't frozen bitrate upgrades due to excessive 
      // bitrate upgrade in the last BwUpgradeFreezeTime millisecods, we upgrade the bitrate.
      else if (is_bw_stable(nacks_to_frames_ratio, avg_rtt, avg_frames_dropped_per_second, queue_fullness))
      {
        if (get_ms_elapsed_since(&bw_upgrade_freeze_start_time) > 180000)
        {
          uint64_t recommended_bitrate = compute_recommended_bitrate(current_encoding_bitrate, params->max_encoding_bitrate, params->min_encoding_bitrate, FTL_BANDWIDTH_AVAILABLE);
          if (recommended_bitrate != current_encoding_bitrate)
          {
            attempt_to_revert_to_stable_bandwidth_first = TRUE;

            BOOL changeBitrateResult = params->change_bitrate_callback(params->context, recommended_bitrate);
            if (changeBitrateResult)
            {
              ftl_bitrate_changed_msg_t msg =
              {
                FTL_BITRATE_INCREASED,
                FTL_BANDWIDTH_AVAILABLE,
                recommended_bitrate,
                current_encoding_bitrate,
                nacks_to_frames_ratio,
                avg_rtt,
                avg_frames_dropped_per_second,
                queue_fullness
              };
              ftl_status_msg_t status_msg;
              status_msg.type = FTL_BITRATE_CHANGED;
              status_msg.msg.bitrate_changed_msg = msg;
              enqueue_status_msg(params->handle->priv, &status_msg);

              bitrate_changed = TRUE;
              current_encoding_bitrate = recommended_bitrate;
              // We have reached a MAX_SUPPORTED_BITRATE_BPS. Check for stbility.
              if (recommended_bitrate == params->max_encoding_bitrate)
              {
                check_bitrate_for_stability = TRUE;
              }
              gettimeofday(&last_bitrate_upgrade_time, NULL);
            }
          }
        }
      }
      // If bitrate was changed by one of the steps above, 
      // we sleep for a cooldown period and clear our circular buffers.
      if (bitrate_changed)
      {
        // Clear out the circular buffer as we dont want it to impact our calculations further.
        circular_buffer_is_full = FALSE;
        current_position_of_circular_buffer = 0;

        // set the peak kbps for throttling
        ftl_media_component_common_t *video = &ftl->video.media_component;
        video->peak_kbps = (int)(5 * current_encoding_bitrate / 1000);

        // Sleep for a c_ulBitrateChangedCooldownIntervalMs period. If hBroadcastTerminated signal is received exit.
        os_semaphore_pend(&ftl->bitrate_thread_shutdown, BITRATE_CHANGED_COOLDOWN_INTERVAL_MS);
        if (!ftl_get_state(params->handle->priv, FTL_BITRATE_THRD))
        {
          break;
        }
        // Update ullLastFramesSentRecorded and ullLastNacksReceivedRecorded, so the cool down has no impact on our calculations.
        ftl_get_video_stats(params->handle, &last_frames_sent_recorded, &last_nacks_received_recorded, &rtt_received, &last_frames_dropped_recorded, &queue_fullness);
        bitrate_changed = FALSE;
      }
      else
      {
        if (check_bitrate_for_stability)
        {
          FTL_LOG(params->handle->priv, FTL_LOG_INFO, "Stable Bitrate acheived");

          check_bitrate_for_stability = FALSE;
          if (current_encoding_bitrate == params->max_encoding_bitrate)
          {
            //Uncomment the following 3 lines to demo adaptive bitrate
            //FTL_LOG(params->handle->priv, FTL_LOG_INFO, "Zapping back to low value for demo.");
            //current_encoding_bitrate = 512 * 1000;
            //params->change_bitrate_callback(params->context, 512 * 1000);

            ftl_bitrate_changed_msg_t msg =
            {
              FTL_BITRATE_STABILIZED,
              FTL_STABILIZE_ON_ORIGINAL_BITRATE,
              params->max_encoding_bitrate,
              current_encoding_bitrate,
              nacks_to_frames_ratio,
              avg_rtt,
              avg_frames_dropped_per_second,
              queue_fullness
            };
            ftl_status_msg_t status_msg;
            status_msg.type = FTL_BITRATE_CHANGED;
            status_msg.msg.bitrate_changed_msg = msg;
            enqueue_status_msg(params->handle->priv, &status_msg);
          }
          else
          {
            ftl_bitrate_changed_msg_t msg =
            {
              FTL_BITRATE_STABILIZED,
              FTL_STABILIZE_ON_LOWER_BITRATE,
              current_encoding_bitrate,
              current_encoding_bitrate,
              nacks_to_frames_ratio,
              avg_rtt,
              avg_frames_dropped_per_second,
              queue_fullness
            };
            ftl_status_msg_t status_msg;
            status_msg.type = FTL_BITRATE_CHANGED;
            status_msg.msg.bitrate_changed_msg = msg;
            enqueue_status_msg(params->handle->priv, &status_msg);
          }
        }
      }
    }

    // Sleep for c_ulStreamStatsCaptureMs before capturing the next stats
    os_semaphore_pend(&ftl->bitrate_thread_shutdown, STREAM_STATS_CAPTURE_MS);
    if (!ftl_get_state(params->handle->priv, FTL_BITRATE_THRD))
    {
      break;
    }
  }
  FTL_LOG(params->handle->priv, FTL_LOG_INFO, "Shutting down bitrate thread");
  free(params);
  return 0;
}
