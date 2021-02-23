/* Copyright Dr. Alex. Gouaillard (2015, 2020) */

#include <obs-module.h>

struct webrtc_custom {
	char *server;
	char *password;
	char *codec;
	bool simulcast;
	char *output;
};

static const char *webrtc_custom_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Custom WebRTC Streaming Platform";
}

static void webrtc_custom_update(void *data, obs_data_t *settings)
{
	struct webrtc_custom *service = data;

	bfree(service->server);
	bfree(service->password);
	bfree(service->codec);
	bfree(service->output);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->codec = bstrdup(obs_data_get_string(settings, "codec"));
	service->simulcast = obs_data_get_bool(settings, "simulcast");
	service->output = bstrdup("webrtc_custom_output");
}

static void webrtc_custom_destroy(void *data)
{
	struct webrtc_custom *service = data;

	bfree(service->server);
	bfree(service->password);
	bfree(service->codec);
	bfree(service->output);
	bfree(service);
}

static void *webrtc_custom_create(obs_data_t *settings, obs_service_t *service)
{
	struct webrtc_custom *data = bzalloc(sizeof(struct webrtc_custom));
	webrtc_custom_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static bool use_auth_modified(obs_properties_t *ppts, obs_property_t *p,
			      obs_data_t *settings)
{
	p = obs_properties_get(ppts, "server");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "key");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "room");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "username");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "password");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "codec");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "protocol");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "simulcast");
	obs_property_set_visible(p, true);

	return true;
}

static obs_properties_t *webrtc_custom_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(ppts, "server", "Publish API URL",
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password", "Auth Bearer Token",
				OBS_TEXT_PASSWORD);

	obs_properties_add_text(ppts, "codec", "Codec", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "protocol", "Protocol", OBS_TEXT_DEFAULT);

	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "AV1",
				     "av1");

	p = obs_properties_get(ppts, "server");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "key");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "username");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "password");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "codec");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "protocol");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "simulcast");
	obs_property_set_visible(p, true);

	// obs_property_set_modified_callback(p, use_auth_modified);

	return ppts;
}

static const char *webrtc_custom_url(void *data)
{
	struct webrtc_custom *service = data;
	return service->server;
}

static const char *webrtc_custom_key(void *data)
{
	UNUSED_PARAMETER(data);
	return NULL;
}

static const char *webrtc_custom_room(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_custom_username(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_custom_password(void *data)
{
	struct webrtc_custom *service = data;
	return service->password;
}

static const char *webrtc_custom_codec(void *data)
{
	struct webrtc_custom *service = data;
	if (strcmp(service->codec, "Automatic") == 0)
		return "";
	return service->codec;
}

static const char *webrtc_custom_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static bool webrtc_custom_simulcast(void *data)
{
	struct webrtc_custom *service = data;
	return service->simulcast;
}

static const char *webrtc_custom_get_output_type(void *data)
{
	struct webrtc_custom *service = data;
	return service->output;
}

struct obs_service_info webrtc_custom_service = {
	.id = "webrtc_custom",
	.get_name = webrtc_custom_name,
	.create = webrtc_custom_create,
	.destroy = webrtc_custom_destroy,
	.update = webrtc_custom_update,
	.get_properties = webrtc_custom_properties,
	.get_url = webrtc_custom_url,
	.get_key = webrtc_custom_key,
	.get_room = webrtc_custom_room,
	.get_username = webrtc_custom_username,
	.get_password = webrtc_custom_password,
	.get_codec = webrtc_custom_codec,
	.get_protocol = webrtc_custom_protocol,
	.get_simulcast = webrtc_custom_simulcast,
	.get_output_type = webrtc_custom_get_output_type};
