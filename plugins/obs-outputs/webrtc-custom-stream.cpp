// Copyright Dr. Alex. Gouaillard (2015, 2020)

#include <stdio.h>
#include <obs-module.h>
#include <obs-avc.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/threading.h>
#include <inttypes.h>

#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)

#define OPT_DROP_THRESHOLD "drop_threshold_ms"
#define OPT_PFRAME_DROP_THRESHOLD "pframe_drop_threshold_ms"
#define OPT_MAX_SHUTDOWN_TIME_SEC "max_shutdown_time_sec"
#define OPT_BIND_IP "bind_ip"
#define OPT_NEWSOCKETLOOP_ENABLED "new_socket_loop_enabled"
#define OPT_LOWLATENCY_ENABLED "low_latency_mode_enabled"

#include "WebRTCStream.h"

extern "C" const char *webrtc_custom_stream_getname(void *unused)
{
	info("webrtc_custom_stream_getname");
	UNUSED_PARAMETER(unused);
	return obs_module_text("webrtc_customStream");
}

extern "C" void webrtc_custom_stream_destroy(void *data)
{
	info("webrtc_custom_stream_destroy");
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	//Stop it
	stream->stop();
	//Remove ref and let it self destroy
	stream->Release();
}

extern "C" void *webrtc_custom_stream_create(obs_data_t *, obs_output_t *output)
{
	info("webrtc_custom_stream_create");
	// Create new stream
	WebRTCStream *stream = new WebRTCStream(output);
	// Don't allow it to be deleted
	stream->AddRef();
	// info("webrtc_custom_setCodec: h264");
	// stream->setCodec("h264");
	// Return it
	return (void *)stream;
}

extern "C" void webrtc_custom_stream_stop(void *data, uint64_t ts)
{
	info("webrtc_custom_stream_stop");
	UNUSED_PARAMETER(ts);
	// Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	// Stop it
	stream->stop();
	// Remove ref and let it self destroy
	stream->Release();
}

extern "C" bool webrtc_custom_stream_start(void *data)
{
	info("webrtc_custom_stream_start");
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	//Don't allow it to be deleted
	stream->AddRef();
	//Start it
	return stream->start(WebRTCStream::Type::CustomWebrtc);
}

extern "C" void webrtc_custom_receive_video(void *data,
					    struct video_data *frame)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	//Process audio
	stream->onVideoFrame(frame);
}
extern "C" void webrtc_custom_receive_audio(void *data,
					    struct audio_data *frame)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	//Process audio
	stream->onAudioFrame(frame);
}

extern "C" void webrtc_custom_stream_defaults(obs_data_t *defaults)
{
	info("webrtc_custom_stream_defaults");
	obs_data_set_default_int(defaults, OPT_DROP_THRESHOLD, 700);
	obs_data_set_default_int(defaults, OPT_PFRAME_DROP_THRESHOLD, 900);
	obs_data_set_default_int(defaults, OPT_MAX_SHUTDOWN_TIME_SEC, 30);
	obs_data_set_default_string(defaults, OPT_BIND_IP, "default");
	obs_data_set_default_bool(defaults, OPT_NEWSOCKETLOOP_ENABLED, false);
	obs_data_set_default_bool(defaults, OPT_LOWLATENCY_ENABLED, false);
}

extern "C" obs_properties_t *webrtc_custom_stream_properties(void *unused)
{
	info("webrtc_custom_stream_properties");
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(
		props, OPT_DROP_THRESHOLD,
		obs_module_text("webrtc_customStream.DropThreshold"), 200,
		10000, 100);

	obs_properties_add_bool(
		props, OPT_NEWSOCKETLOOP_ENABLED,
		obs_module_text("webrtc_customStream.NewSocketLoop"));
	obs_properties_add_bool(
		props, OPT_LOWLATENCY_ENABLED,
		obs_module_text("webrtc_customStream.LowLatencyMode"));

	return props;
}

// NOTE LUDO: #80 add getStats
extern "C" void webrtc_custom_stream_get_stats(void *data)
{
	// Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	stream->getStats();
}

extern "C" const char *webrtc_custom_stream_get_stats_list(void *data)
{
	// Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->get_stats_list();
}

extern "C" uint64_t webrtc_custom_stream_total_bytes_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getBitrate();
}

extern "C" int webrtc_custom_stream_dropped_frames(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getDroppedFrames();
}

// #310 webrtc getstats()
extern "C" uint64_t webrtc_custom_stream_get_transport_bytes_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTransportBytesSent();
}

extern "C" uint64_t
webrtc_custom_stream_get_transport_bytes_received(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTransportBytesReceived();
}

extern "C" uint64_t webrtc_custom_stream_get_video_packets_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getVideoPacketsSent();
}

extern "C" uint64_t webrtc_custom_stream_get_video_bytes_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getVideoBytesSent();
}

extern "C" uint64_t webrtc_custom_stream_get_video_fir_count(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getVideoFirCount();
}

extern "C" uint32_t webrtc_custom_stream_get_video_pli_count(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getVideoPliCount();
}

extern "C" uint64_t webrtc_custom_stream_get_video_nack_count(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getVideoNackCount();
}

extern "C" uint64_t webrtc_custom_stream_get_video_qp_sum(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getVideoQpSum();
}

extern "C" uint64_t webrtc_custom_stream_get_audio_packets_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getAudioPacketsSent();
}

extern "C" uint64_t webrtc_custom_stream_get_audio_bytes_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getAudioBytesSent();
}

extern "C" uint32_t webrtc_custom_stream_get_track_audio_level(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTrackAudioLevel();
}

extern "C" uint32_t
webrtc_custom_stream_get_track_total_audio_energy(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTrackTotalAudioEnergy();
}

extern "C" uint32_t
webrtc_custom_stream_get_track_total_samples_duration(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTrackTotalSamplesDuration();
}

extern "C" uint32_t webrtc_custom_stream_get_track_frame_width(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTrackFrameWidth();
}

extern "C" uint32_t webrtc_custom_stream_get_track_frame_height(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTrackFrameHeight();
}

extern "C" uint64_t webrtc_custom_stream_get_track_frames_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTrackFramesSent();
}

extern "C" uint64_t webrtc_custom_stream_get_track_huge_frames_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getTrackHugeFramesSent();
}

extern "C" float webrtc_custom_stream_congestion(void *data)
{
	UNUSED_PARAMETER(data);
	return 0.0f;
}

extern "C" {
// #ifdef _WIN32
// struct obs_output_info webrtc_custom_output_info = {
// 	"webrtc_custom_output",             //id
// 	OBS_OUTPUT_AV | OBS_OUTPUT_SERVICE, //flags
// 	webrtc_custom_stream_getname,       //get_name
// 	webrtc_custom_stream_create,        //create
// 	webrtc_custom_stream_destroy,       //destroy
// 	webrtc_custom_stream_start,         //start
// 	webrtc_custom_stream_stop,          //stop
// 	webrtc_custom_receive_video,        //raw_video
// 	webrtc_custom_receive_audio,        //raw_audio
// 	nullptr,                            //encoded_packet
// 	nullptr,                            //update
// 	webrtc_custom_stream_defaults,      //get_defaults
// 	webrtc_custom_stream_properties,    //get_properties
// 	nullptr,                            //unused1 (formerly pause)
// 	// NOTE LUDO: #80 add getStats
// 	webrtc_custom_stream_get_stats, webrtc_custom_stream_get_stats_list,
// 	webrtc_custom_stream_total_bytes_sent, //get_total_bytes
// 	webrtc_custom_stream_dropped_frames,   //get_dropped_frames
// 	// #310 webrtc getstats()
// 	webrtc_custom_stream_get_transport_bytes_sent, //get_transport_bytes_sent
// 	webrtc_custom_stream_get_transport_bytes_received, //get_transport_bytes_received
// 	webrtc_custom_stream_get_video_packets_sent, //get_video_packets_sent
// 	webrtc_custom_stream_get_video_bytes_sent,   //get_video_bytes_sent
// 	webrtc_custom_stream_get_video_fir_count,    //get_video_fir_count
// 	webrtc_custom_stream_get_video_pli_count,    //get_video_pli_count
// 	webrtc_custom_stream_get_video_nack_count,   //get_video_nack_count
// 	webrtc_custom_stream_get_video_qp_sum,       //get_video_qp_sum
// 	webrtc_custom_stream_get_audio_packets_sent, //get_audio_packets_sent
// 	webrtc_custom_stream_get_audio_bytes_sent,   //get_audio_bytes_sent
// 	webrtc_custom_stream_get_track_audio_level,  //get_track_audio_level
// 	webrtc_custom_stream_get_track_total_audio_energy, //get_track_total_audio_energy
// 	webrtc_custom_stream_get_track_total_samples_duration, //get_track_total_samples_duration
// 	webrtc_custom_stream_get_track_frame_width,  //get_track_frame_width
// 	webrtc_custom_stream_get_track_frame_height, //get_track_frame_height
// 	webrtc_custom_stream_get_track_frames_sent,  //get_track_frames_sent
// 	webrtc_custom_stream_get_track_huge_frames_sent, //get_track_huge_frames_sent
// 	nullptr,                                         //type_data
// 	nullptr,                                         //free_type_data
// 	webrtc_custom_stream_congestion,                 //get_congestion
// 	nullptr,                                         //get_connect_time_ms
// 	"vp8",                                           //encoded_video_codecs
// 	"opus",                                          //encoded_audio_codecs
// 	nullptr                                          //raw_audio2
// };
// #else
struct obs_output_info webrtc_custom_output_info = {
	.id = "webrtc_custom_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_SERVICE,
	.protocols = "WebRTC",
	.encoded_video_codecs = "h264;vp8;vp9;av1",
	.encoded_audio_codecs = "opus",
	.get_name = webrtc_custom_stream_getname,
	.create = webrtc_custom_stream_create,
	.destroy = webrtc_custom_stream_destroy,
	.start = webrtc_custom_stream_start,
	.stop = webrtc_custom_stream_stop,
	.raw_video = webrtc_custom_receive_video,
	.raw_audio = webrtc_custom_receive_audio, //for single-track
	.encoded_packet = nullptr,
	.update = nullptr,
	.get_defaults = webrtc_custom_stream_defaults,
	.get_properties = webrtc_custom_stream_properties,
	.unused1 = nullptr,
	// NOTE LUDO: #80 add getStats
	.get_stats = webrtc_custom_stream_get_stats,
	.get_stats_list = webrtc_custom_stream_get_stats_list,
	.get_total_bytes = webrtc_custom_stream_total_bytes_sent,
	.get_dropped_frames = webrtc_custom_stream_dropped_frames,
	// #310 webrtc getstats()
	.get_transport_bytes_sent =
		webrtc_custom_stream_get_transport_bytes_sent,
	.get_transport_bytes_received =
		webrtc_custom_stream_get_transport_bytes_received,
	.get_video_packets_sent = webrtc_custom_stream_get_video_packets_sent,
	.get_video_bytes_sent = webrtc_custom_stream_get_video_bytes_sent,
	.get_video_fir_count = webrtc_custom_stream_get_video_fir_count,
	.get_video_pli_count = webrtc_custom_stream_get_video_pli_count,
	.get_video_nack_count = webrtc_custom_stream_get_video_nack_count,
	.get_video_qp_sum = webrtc_custom_stream_get_video_qp_sum,
	.get_audio_packets_sent = webrtc_custom_stream_get_audio_packets_sent,
	.get_audio_bytes_sent = webrtc_custom_stream_get_audio_bytes_sent,
	.get_track_audio_level = webrtc_custom_stream_get_track_audio_level,
	.get_track_total_audio_energy =
		webrtc_custom_stream_get_track_total_audio_energy,
	.get_track_total_samples_duration =
		webrtc_custom_stream_get_track_total_samples_duration,
	.get_track_frame_width = webrtc_custom_stream_get_track_frame_width,
	.get_track_frame_height = webrtc_custom_stream_get_track_frame_height,
	.get_track_frames_sent = webrtc_custom_stream_get_track_frames_sent,
	.get_track_huge_frames_sent =
		webrtc_custom_stream_get_track_huge_frames_sent,
	.type_data = nullptr,
	.free_type_data = nullptr,
	.get_congestion = webrtc_custom_stream_congestion,
	.get_connect_time_ms = nullptr,
	.encoded_video_codecs = "vp8",
	.encoded_audio_codecs = "opus",
	.raw_audio2 = nullptr
	// .raw_audio2           = webrtc_custom_receive_multitrack_audio, //for multi-track
};
// #endif
}
