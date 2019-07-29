#include<obs-module.h>

struct webrtc_millicast {
	char *server;
	char *username;
	char *password;
	char *codec;
	char *output;
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
	bfree(service->username);
	bfree(service->codec);
	bfree(service->password);
	bfree(service->output);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->username = bstrdup(obs_data_get_string(settings, "username"));
	service->password = bstrdup(obs_data_get_string(settings, "password"));
	service->codec = bstrdup(obs_data_get_string(settings, "codec"));
	service->output = NULL;

	if (!service->output)
		service->output = bstrdup("millicast_output");

}

static void webrtc_millicast_destroy(void *data)
{
	struct webrtc_millicast *service = data;

	bfree(service->server);
	bfree(service->username);
	bfree(service->codec);
	bfree(service->password);
	bfree(service->output);
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

	obs_properties_add_list(ppts, "server", obs_module_text("Server"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(obs_properties_get(ppts, "server"), "Auto (Recommended)", "wss://live.millicast.com:443/ws/v1/pub");

	obs_properties_add_text(ppts, "username", "Publishing Stream Name", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password", obs_module_text("Publishing Token"), OBS_TEXT_PASSWORD);

	obs_properties_add_list(ppts, "codec", obs_module_text("Codec"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "Automatic", "");
	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "H264", "h264");
	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "VP8", "vp8");
	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "VP9", "vp9");

	return ppts;
}

static const char *webrtc_millicast_url(void *data)
{
	struct webrtc_millicast *service = data;
	return service->server;
}

static const char *webrtc_millicast_room(void *data)
{
	UNUSED_PARAMETER(data);
	return "2";
}

static const char *webrtc_millicast_key(void *data)
{
	UNUSED_PARAMETER(data);
	return NULL;
}

static const char *webrtc_millicast_username(void *data)
{
	struct webrtc_millicast *service = data;
	return service->username;
}

static const char *webrtc_millicast_codec(void *data)
{
	struct webrtc_millicast *service = data;
	return service->codec;
}

static const char *webrtc_millicast_password(void *data)
{
	struct webrtc_millicast *service = data;
	return service->password;
}

static const char *webrtc_millicast_protocol(void *data)
{
	// struct webrtc_millicast *service = data;
	// return service->protocol;

	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_millicast_get_output_type(void *data)
{
	struct webrtc_millicast *service = data;
	return service->output;
}

struct obs_service_info webrtc_millicast_service = {
	.id              = "webrtc_millicast",
	.get_name        = webrtc_millicast_name,
	.create          = webrtc_millicast_create,
	.destroy         = webrtc_millicast_destroy,
	.update          = webrtc_millicast_update,
	.get_properties  = webrtc_millicast_properties,
	.get_url         = webrtc_millicast_url,
	.get_room        = webrtc_millicast_room,
	.get_key         = webrtc_millicast_key,
	.get_username    = webrtc_millicast_username,
	.get_password    = webrtc_millicast_password,
	.get_codec       = webrtc_millicast_codec,
	.get_protocol    = webrtc_millicast_protocol,
	.get_output_type = webrtc_millicast_get_output_type
};
