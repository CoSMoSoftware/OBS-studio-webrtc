#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-outputs", "en-US")

extern struct obs_output_info flv_output_info;
extern struct obs_output_info rtmp_output_info;
extern struct obs_output_info null_output_info;

extern const char *janus_stream_getname(void *unused);
extern void janus_stream_destroy(void *data);
extern void *janus_stream_create(obs_data_t *settings, obs_output_t *output);
extern void janus_stream_stop(void *data, uint64_t ts);
extern bool janus_stream_start(void *data);
extern void janus_receive_video(void *param, struct video_data *frame);
extern void janus_receive_audio(void *param, struct audio_data *frame);
extern void janus_stream_defaults(obs_data_t *defaults);
extern obs_properties_t *janus_stream_properties(void *unused);
extern uint64_t janus_stream_total_bytes_sent(void *data);
extern int janus_stream_dropped_frames(void *data);
extern float janus_stream_congestion(void *data);

struct obs_output_info janus_output_info = {
	.id = "janus_output",
	.flags = OBS_OUTPUT_AV |
	OBS_OUTPUT_SERVICE |
	OBS_OUTPUT_MULTI_TRACK,
	.encoded_video_codecs = "vp8",
	.encoded_audio_codecs = "opus",
	.get_name = janus_stream_getname,
	.create = janus_stream_create,
	.destroy = janus_stream_destroy,
	.start = janus_stream_start,
	.stop = janus_stream_stop,
	.raw_video = janus_receive_video,
	.raw_audio = janus_receive_audio,
	.get_defaults = janus_stream_defaults,
	.get_properties = janus_stream_properties,
	.get_total_bytes = janus_stream_total_bytes_sent,
	.get_congestion = janus_stream_congestion,
	.get_dropped_frames = janus_stream_dropped_frames
};

extern const char *spankchain_stream_getname(void *unused);
extern void spankchain_stream_destroy(void *data);
extern void *spankchain_stream_create(obs_data_t *settings, obs_output_t *output);
extern void spankchain_stream_stop(void *data, uint64_t ts);
extern bool spankchain_stream_start(void *data);
extern void spankchain_receive_video(void *param, struct video_data *frame);
extern void spankchain_receive_audio(void *param, struct audio_data *frame);
extern void spankchain_stream_defaults(obs_data_t *defaults);
extern obs_properties_t *spankchain_stream_properties(void *unused);
extern uint64_t spankchain_stream_total_bytes_sent(void *data);
extern int spankchain_stream_dropped_frames(void *data);
extern float spankchain_stream_congestion(void *data);

struct obs_output_info spankchain_output_info = {
	.id = "spankchain_output",
	.flags = OBS_OUTPUT_AV |
	OBS_OUTPUT_SERVICE |
	OBS_OUTPUT_MULTI_TRACK,
	.encoded_video_codecs = "vp8",
	.encoded_audio_codecs = "opus",
	.get_name = spankchain_stream_getname,
	.create = spankchain_stream_create,
	.destroy = spankchain_stream_destroy,
	.start = spankchain_stream_start,
	.stop = spankchain_stream_stop,
	.raw_video = spankchain_receive_video,
	.raw_audio = spankchain_receive_audio,
	.get_defaults = spankchain_stream_defaults,
	.get_properties = spankchain_stream_properties,
	.get_total_bytes = spankchain_stream_total_bytes_sent,
	.get_congestion = spankchain_stream_congestion,
	.get_dropped_frames = spankchain_stream_dropped_frames
};

bool obs_module_load(void)
{
	obs_register_output(&flv_output_info);
	obs_register_output(&rtmp_output_info);
	obs_register_output(&null_output_info);
	obs_register_output(&janus_output_info);
	obs_register_output(&spankchain_output_info);
	return true;
}

void obs_module_unload(void)
{

}
