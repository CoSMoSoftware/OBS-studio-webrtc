#include<obs-module.h>

struct webrtc_spankchain {
    char *server, *api, *token;
};

static const char *webrtc_spankchain_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WebRTC Spankchain Streaming Server");
}

static void webrtc_spankchain_update(void *data, obs_data_t *settings)
{
	struct webrtc_spankchain *service = data;

    bfree(service->server);
    bfree(service->api);
	bfree(service->token);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
    service->api = bstrdup(obs_data_get_string(settings, "api"));
	service->token = bstrdup(obs_data_get_string(settings, "token"));
}

static void webrtc_spankchain_destroy(void *data)
{
	struct webrtc_spankchain *service = data;

	bfree(service->server);
    bfree(service->api);
	bfree(service->token);
	bfree(service);
}

static void *webrtc_spankchain_create(obs_data_t *settings, obs_service_t *service)
{
	struct webrtc_spankchain *data = bzalloc(sizeof(struct webrtc_spankchain));
	webrtc_spankchain_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t *webrtc_spankchain_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "server", "Web Server URL", OBS_TEXT_DEFAULT);
    obs_properties_add_text(ppts, "api", "API URL", OBS_TEXT_DEFAULT);
    obs_properties_add_text(ppts, "token", obs_module_text("Token"),OBS_TEXT_PASSWORD);

	return ppts;
}

static const char *webrtc_spankchain_url(void *data)
{
	struct webrtc_spankchain *service = data;
	return service->server;
}

static const char *webrtc_spankchain_room(void *data)
{
	return "1";
}

static const char *webrtc_spankchain_token(void *data)
{
	struct webrtc_spankchain *service = data;
	return service->token;
}

static const char *webrtc_spankchain_username(void *data)
{
    struct webrtc_spankchain *service = data;
    return service->api;
}

struct obs_service_info webrtc_spankchain_service = {
	.id             = "webrtc_spankchain",
	.get_name       = webrtc_spankchain_name,
	.create         = webrtc_spankchain_create,
	.destroy        = webrtc_spankchain_destroy,
	.update         = webrtc_spankchain_update,
	.get_properties = webrtc_spankchain_properties,
	.get_url        = webrtc_spankchain_url,
	.get_room       = webrtc_spankchain_room,
    .get_username   = webrtc_spankchain_username,
	.get_password   = webrtc_spankchain_token
};
