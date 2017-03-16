#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-outputs", "en-US")

static const char *flv_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FLVOutput");
}

static void flv_output_stop(void *data, uint64_t ts)
{

}

static void flv_output_destroy(void *data)
{
}

static void *flv_output_create(obs_data_t *settings, obs_output_t *output)
{
	return NULL;
}

static bool flv_output_start(void *data)
{
	return true;
}

static void flv_output_data(void *data, struct encoder_packet *packet)
{

}

static obs_properties_t *flv_output_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "path",
		obs_module_text("FLVOutput.FilePath"),
		OBS_TEXT_DEFAULT);
	return props;
}


struct obs_output_info flv_output_info = {
	.id = "flv_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED,
	.get_name = flv_output_getname,
	.create = flv_output_create,
	.destroy = flv_output_destroy,
	.start = flv_output_start,
	.stop = flv_output_stop,
	.encoded_packet = flv_output_data,
	.get_properties = flv_output_properties
};


extern const char *rtmp_stream_getname(void *unused);
extern void rtmp_stream_destroy(void *data);
extern void *rtmp_stream_create(obs_data_t *settings, obs_output_t *output);
extern void rtmp_stream_stop(void *data, uint64_t ts);
extern bool rtmp_stream_start(void *data);
extern void rtmp_receive_video(void *param, struct video_data *frame);
extern void rtmp_receive_audio(void *param, struct audio_data *frame);
extern void rtmp_stream_defaults(obs_data_t *defaults);
extern obs_properties_t *rtmp_stream_properties(void *unused);

extern uint64_t rtmp_stream_total_bytes_sent(void *data);
extern int rtmp_stream_dropped_frames(void *data);
extern float rtmp_stream_congestion(void *data);


struct obs_output_info rtmp_output_info = {
	.id = "rtmp_output",
	.flags = OBS_OUTPUT_AV |
	OBS_OUTPUT_SERVICE |
	OBS_OUTPUT_MULTI_TRACK,
	.get_name = rtmp_stream_getname,
	.create = rtmp_stream_create,
	.destroy = rtmp_stream_destroy,
	.start = rtmp_stream_start,
	.stop = rtmp_stream_stop,
	.raw_video = rtmp_receive_video,
	.raw_audio = rtmp_receive_audio,
	.get_defaults = rtmp_stream_defaults,
	.get_properties = rtmp_stream_properties,
	.get_total_bytes = rtmp_stream_total_bytes_sent,
	.get_congestion = rtmp_stream_congestion,
	.get_dropped_frames = rtmp_stream_dropped_frames
};



bool obs_module_load(void)
{

	obs_register_output(&rtmp_output_info);
	obs_register_output(&flv_output_info);
	return true;
}

void obs_module_unload(void)
{

}
