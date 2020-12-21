/* Copyright Dr. Alex. Gouaillard (2015, 2020) */

#include <obs-module.h>

struct webrtc_millicast {
	char *server;
	char *username;
	char *password;
	char *codec;
	bool simulcast;
	char *publishApiUrl;
	char *output;
};

static const char *webrtc_millicast_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Millicast WebRTC Streaming Platform";
}

static void webrtc_millicast_update(void *data, obs_data_t *settings)
{
	struct webrtc_millicast *service = data;

	bfree(service->server);
	bfree(service->username);
	bfree(service->codec);
	bfree(service->password);
	bfree(service->publishApiUrl);
	bfree(service->output);

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->username = bstrdup(obs_data_get_string(settings, "username"));
	service->password = bstrdup(obs_data_get_string(settings, "password"));
	service->codec = bstrdup(obs_data_get_string(settings, "codec"));
	service->simulcast = obs_data_get_bool(settings, "simulcast");
	service->publishApiUrl =
		bstrdup(obs_data_get_string(settings, "publish_api_url"));
	service->output = bstrdup("millicast_output");
}

static void webrtc_millicast_destroy(void *data)
{
	struct webrtc_millicast *service = data;

	bfree(service->server);
	bfree(service->username);
	bfree(service->codec);
	bfree(service->password);
	bfree(service->publishApiUrl);
	bfree(service->output);
	bfree(service);
}

static void *webrtc_millicast_create(obs_data_t *settings,
				     obs_service_t *service)
{
	struct webrtc_millicast *data =
		bzalloc(sizeof(struct webrtc_millicast));
	webrtc_millicast_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static bool use_auth_modified(obs_properties_t *ppts, obs_property_t *p,
			      obs_data_t *settings)
{
	p = obs_properties_get(ppts, "server");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "key");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "room");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "username");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "password");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "codec");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "protocol");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "simulcast");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "publish_api_url");
	obs_property_set_visible(p, true);

	return true;
}

static obs_properties_t *webrtc_millicast_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(ppts, "room", "Room", OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "username", "Stream Name",
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password", "Publishing Token",
				OBS_TEXT_PASSWORD);

	obs_properties_add_text(ppts, "codec", "Codec", OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "protocol", "Protocol", OBS_TEXT_DEFAULT);

	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "AV1",
				     "av1");

	obs_properties_add_text(ppts, "publish_api_url", "Publish API URL",
				OBS_TEXT_DEFAULT);

	p = obs_properties_get(ppts, "server");
	obs_property_set_visible(p, false);

	obs_property_list_add_string(
		obs_properties_get(ppts, "server"), "Auto (Recommended)",
		"https://director.millicast.com/api/director/publish");
	// NOTE ALEX: the default version of libwebrtc does not include support for H264 in software.
	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "h264",
				     "h264");
	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "vp8",
				     "vp8");
	obs_property_list_add_string(obs_properties_get(ppts, "codec"), "vp9",
				     "vp9");

	p = obs_properties_get(ppts, "room");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "username");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "password");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "codec");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "protocol");
	obs_property_set_visible(p, false);

	p = obs_properties_get(ppts, "simulcast");
	obs_property_set_visible(p, true);

	p = obs_properties_get(ppts, "publish_api_url");
	obs_property_set_visible(p, true);

	// obs_property_set_modified_callback(p, use_auth_modified);

	return ppts;
}

static const char *webrtc_millicast_url(void *data)
{
	struct webrtc_millicast *service = data;
	return service->server;
}

static const char *webrtc_millicast_key(void *data)
{
	UNUSED_PARAMETER(data);
	return NULL;
}

static const char *webrtc_millicast_room(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_millicast_username(void *data)
{
	struct webrtc_millicast *service = data;
	return service->username;
}

static const char *webrtc_millicast_password(void *data)
{
	struct webrtc_millicast *service = data;
	return service->password;
}

static const char *webrtc_millicast_codec(void *data)
{
	struct webrtc_millicast *service = data;
	if (strcmp(service->codec, "Automatic") == 0)
		return "";
	return service->codec;
}

static bool webrtc_millicast_simulcast(void *data)
{
	struct webrtc_millicast *service = data;
	return service->simulcast;
}

static const char *webrtc_millicast_publishApiUrl(void *data)
{
	struct webrtc_millicast *service = data;
	if (strlen(service->publishApiUrl) == 0) {
		return "https://director.millicast.com/api/director/publish";
	}

	return service->publishApiUrl;
}

static const char *webrtc_millicast_protocol(void *data)
{
	UNUSED_PARAMETER(data);
	return "";
}

static const char *webrtc_millicast_get_output_type(void *data)
{
	struct webrtc_millicast *service = data;
	return service->output;
}

struct obs_service_info webrtc_millicast_service = {
	.id = "webrtc_millicast",
	.get_name = webrtc_millicast_name,
	.create = webrtc_millicast_create,
	.destroy = webrtc_millicast_destroy,
	.update = webrtc_millicast_update,
	.get_properties = webrtc_millicast_properties,
	.get_url = webrtc_millicast_url,
	.get_key = webrtc_millicast_key,
	.get_room = webrtc_millicast_room,
	.get_username = webrtc_millicast_username,
	.get_password = webrtc_millicast_password,
	.get_codec = webrtc_millicast_codec,
	.get_protocol = webrtc_millicast_protocol,
	.get_simulcast = webrtc_millicast_simulcast,
	.get_publishApiUrl = webrtc_millicast_publishApiUrl,
	.get_output_type = webrtc_millicast_get_output_type};
