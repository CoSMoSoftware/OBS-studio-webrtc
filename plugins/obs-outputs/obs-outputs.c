#include <obs-module.h>
#include "janus-stream.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-outputs", "en-US")

extern struct obs_output_info flv_output_info;
extern struct obs_output_info rtmp_output_info;
extern struct obs_output_info null_output_info;
extern struct obs_output_info janus_output_info;
extern struct obs_output_info spankchain_output_info;
extern struct obs_output_info millicast_output_info;

bool obs_module_load(void)
{
	obs_register_output(&flv_output_info);
	obs_register_output(&rtmp_output_info);
	obs_register_output(&null_output_info);
	obs_register_output(&janus_output_info);
	obs_register_output(&spankchain_output_info);
	obs_register_output(&millicast_output_info);
	return true;
}

void obs_module_unload(void)
{

}
