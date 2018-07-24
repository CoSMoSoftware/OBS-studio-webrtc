#include<obs-module.h>n

struct webrtc_millicast {
    char *server, *token;
};

static const char *webrtc_millicast_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WebRTC Millicast Streaming Server");
}

static void webrtc_millicast_update(void *data, obs_data_t *settings)
{
	struct webrtc_millicast *service = data;

	bfree(service->server);
	bfree(service->token);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->token = bstrdup(obs_data_get_string(settings, "token"));
}

static void webrtc_millicast_destroy(void *data)
{
	struct webrtc_millicast *service = data;

	bfree(service->server);
	bfree(service->token);
	bfree(service);
}

static void *webrtc_millicast_create(obs_data_t *settings, obs_service_t *service)
{
	struct webrtc_millicast *data = bzalloc(sizeof(struct webrtc_millicast));
	webrtc_millicast_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t *webrtc_millicast_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "Server URL", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "token", obs_module_text("Token"),OBS_TEXT_PASSWORD);

	return ppts;
}

static const char *webrtc_millicast_url(void *data)
{
	struct webrtc_millicast *service = data;
	return service->server;
}

static const char *webrtc_millicast_room(void *data)
{
	return "1";
}

static const char *webrtc_millicast_token(void *data)
{
	struct webrtc_millicast *service = data;
	return service->token;
}

struct obs_service_info webrtc_millicast_service = {
	.id             = "webrtc_millicast",
	.get_name       = webrtc_millicast_name,
	.create         = webrtc_millicast_create,
	.destroy        = webrtc_millicast_destroy,
	.update         = webrtc_millicast_update,
	.get_properties = webrtc_millicast_properties,
	.get_url        = webrtc_millicast_url,
	.get_room       = webrtc_millicast_room,
	.get_password   = webrtc_millicast_token
};
