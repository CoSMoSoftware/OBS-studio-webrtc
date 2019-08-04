#include <obs-module.h>
#include "obs-outputs-config.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-outputs", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "OBS core RTMP/FLV/null/FTL/WebRTC outputs";
}

extern struct obs_output_info rtmp_output_info;
extern struct obs_output_info null_output_info;
extern struct obs_output_info flv_output_info;
extern struct obs_output_info janus_output_info;
extern struct obs_output_info wowza_output_info;
extern struct obs_output_info millicast_output_info;
extern struct obs_output_info evercast_output_info;
#if COMPILE_FTL
extern struct obs_output_info ftl_output_info;
#endif

bool obs_module_load(void)
{
#ifdef _WIN32
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 2), &wsad);
#endif

	obs_register_output(&rtmp_output_info);
	obs_register_output(&null_output_info);
	obs_register_output(&flv_output_info);
	obs_register_output(&janus_output_info);
	obs_register_output(&wowza_output_info);
	obs_register_output(&millicast_output_info);
	obs_register_output(&evercast_output_info);
#if COMPILE_FTL
	obs_register_output(&ftl_output_info);
#endif
	return true;
}

void obs_module_unload(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}
