#include <obs-module.h>

struct webrtc_wowza {
    char *server;
    char *token;
    char *codec;
    char *username;
    char *protocol;
};

static const char *webrtc_wowza_name(void *unused)
{
  UNUSED_PARAMETER(unused);
  return obs_module_text("WebRTC Wowza Streaming Engine");
}

static void webrtc_wowza_update(void *data, obs_data_t *settings)
{
  struct webrtc_wowza *service = data;

  bfree(service->server);
  bfree(service->username);
  bfree(service->token);
  bfree(service->codec);
  bfree(service->protocol);

  service->server   = bstrdup(obs_data_get_string(settings, "server"  ));
  service->username = bstrdup(obs_data_get_string(settings, "username"));
  service->token    = bstrdup(obs_data_get_string(settings, "token"   ));
  service->codec    = bstrdup(obs_data_get_string(settings, "codec"   ));
  service->protocol = bstrdup(obs_data_get_string(settings, "protocol"));
}

static void webrtc_wowza_destroy(void *data)
{
  struct webrtc_wowza *service = data;

  bfree(service->server  );
  bfree(service->username);
  bfree(service->token   );
  bfree(service->codec   );
  bfree(service->protocol);
  bfree(service          );
}

static void *webrtc_wowza_create(obs_data_t *settings, obs_service_t *service)
{
  struct webrtc_wowza *data = bzalloc(sizeof(struct webrtc_wowza));
  webrtc_wowza_update(data, settings);

  UNUSED_PARAMETER(service);
  return data;
}

static obs_properties_t *webrtc_wowza_properties(void *unused)
{
  UNUSED_PARAMETER(unused);

  obs_properties_t *ppts = obs_properties_create();

  obs_properties_add_text(ppts, "server", "Server URL", OBS_TEXT_DEFAULT);
  obs_properties_add_text(ppts, "username", "Application Name", OBS_TEXT_DEFAULT);
  obs_properties_add_text(ppts, "token", "Stream Name", OBS_TEXT_DEFAULT);

  obs_properties_add_list(ppts, "codec", "Codec", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
  obs_property_list_add_string(obs_properties_get(ppts, "codec"), "Automatic", "");
  obs_property_list_add_string(obs_properties_get(ppts, "codec"), "H264", "h264");
  obs_property_list_add_string(obs_properties_get(ppts, "codec"), "VP8", "vp8");
  obs_property_list_add_string(obs_properties_get(ppts, "codec"), "VP9", "vp9");

  obs_properties_add_list(ppts, "protocol", "Protocol", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
  obs_property_list_add_string(obs_properties_get(ppts, "protocol"), "Automatic", "");
  obs_property_list_add_string(obs_properties_get(ppts, "protocol"), "UDP", "UDP");
  obs_property_list_add_string(obs_properties_get(ppts, "protocol"), "TCP", "TCP");

  return ppts;
}

static const char *webrtc_wowza_url(void *data)
{
  struct webrtc_wowza *service = data;
  return service->server;
}

static const char *webrtc_wowza_room(void *data)
{
  UNUSED_PARAMETER(data);
  return "2";
}

static const char *webrtc_wowza_username(void *data)
{
  struct webrtc_wowza *service = data;
  return service->username;
}

static const char *webrtc_wowza_token(void *data)
{
  struct webrtc_wowza *service = data;
  return service->token;
}

static const char *webrtc_wowza_codec(void *data)
{
  struct webrtc_wowza *service = data;
  return service->codec;
}

static const char *webrtc_wowza_id(void *data)
{
  UNUSED_PARAMETER(data);
  return "";
}

static const char *webrtc_wowza_protocol(void *data)
{
  struct webrtc_wowza *service = data;
  return service->protocol;
}

struct obs_service_info webrtc_wowza_service = {
  .id             = "webrtc_wowza",
  .get_name       = webrtc_wowza_name,
  .create         = webrtc_wowza_create,
  .destroy        = webrtc_wowza_destroy,
  .update         = webrtc_wowza_update,
  .get_properties = webrtc_wowza_properties,
  .get_url        = webrtc_wowza_url,
  .get_room       = webrtc_wowza_room,
  .get_password   = webrtc_wowza_token,
  .get_codec      = webrtc_wowza_codec,
  .get_username   = webrtc_wowza_username,
  .get_milli_id   = webrtc_wowza_id,
  .get_protocol   = webrtc_wowza_protocol
};
