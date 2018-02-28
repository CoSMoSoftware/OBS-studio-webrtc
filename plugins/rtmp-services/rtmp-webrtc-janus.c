#include<obs-module.h>

struct rtmp_webrtc_janus {
    char *server, *room;
};

static const char *rtmp_webrtc_janus_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("streamingWebRTCJanus");
}

static void rtmp_webrtc_janus_update(void *data, obs_data_t *settings)
{
	struct rtmp_webrtc_janus *service = data;

    bfree(service->server);
	bfree(service->room);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->room = bstrdup(obs_data_get_string(settings, "room"));
}

static void rtmp_webrtc_janus_destroy(void *data)
{
	struct rtmp_webrtc_janus *service = data;

	bfree(service->server);
	bfree(service->room);
	bfree(service);
}

static void *rtmp_webrtc_janus_create(obs_data_t *settings, obs_service_t *service)
{
	struct rtmp_webrtc_janus *data = bzalloc(sizeof(struct rtmp_webrtc_janus));
	rtmp_webrtc_janus_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t *rtmp_webrtc_janus_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "Server Name", OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "room", "Server Room", OBS_TEXT_DEFAULT);



	return ppts;
}

static const char *rtmp_webrtc_janus_url(void *data)
{
	struct rtmp_webrtc_janus *service = data;
	return service->server;
}

static const char *rtmp_webrtc_janus_room(void *data)
{
	struct rtmp_webrtc_janus *service = data;
	return service->room;
}

struct obs_service_info rtmp_webrtc_janus_service = {
	.id             = "rtmp_webrtc_janus",
	.get_name       = rtmp_webrtc_janus_name,
	.create         = rtmp_webrtc_janus_create,
	.destroy        = rtmp_webrtc_janus_destroy,
	.update         = rtmp_webrtc_janus_update,
	.get_properties = rtmp_webrtc_janus_properties,
	.get_url        = rtmp_webrtc_janus_url,
	.get_room        = rtmp_webrtc_janus_room
};