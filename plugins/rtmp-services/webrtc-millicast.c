#include<obs-module.h>

struct webrtc_millicast {
  char *server, *milli_id, *codec, *token;
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
  bfree(service->milli_id);
  bfree(service->codec);
  bfree(service->token);

  service->server   = bstrdup(obs_data_get_string(settings, "server"  ));
  service->milli_id = bstrdup(obs_data_get_string(settings, "milli_id"));
  service->token    = bstrdup(obs_data_get_string(settings, "token"));
  service->codec    = bstrdup(obs_data_get_string(settings, "codec"   ));

}

static void webrtc_millicast_destroy(void *data)
{
  struct webrtc_millicast *service = data;

  bfree(service->server  );
  bfree(service->milli_id);
  bfree(service->codec   );
  bfree(service->token   );
  bfree(service          );
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
  obs_properties_add_text(ppts, "milli_id", "Publishing Stream Name",OBS_TEXT_DEFAULT);
  obs_properties_add_text(ppts, "token", obs_module_text("Publishing token"), OBS_TEXT_PASSWORD);
  obs_properties_add_list(ppts, "codec", obs_module_text("Codec"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

  obs_property_list_add_string(obs_properties_get(ppts, "server"),"Auto (Recommended)", "wss://live.millicast.com:443/ws/v1/pub");
  obs_property_list_add_string(obs_properties_get(ppts, "codec"),"h264", "h264");
  obs_property_list_add_string(obs_properties_get(ppts, "codec"),"vp8", "vp8");
  obs_property_list_add_string(obs_properties_get(ppts, "codec"),"vp9", "vp9");

  return ppts;
}

static const char *webrtc_millicast_url(void *data)
{
  struct webrtc_millicast *service = data;
  return service->server;
}

static const char *webrtc_millicast_id(void *data)
{
  struct webrtc_millicast *service = data;
  return service->milli_id;
}

static const char *webrtc_millicast_codec(void *data)
{
  struct webrtc_millicast *service = data;
  return service->codec;
}

static const char *webrtc_millicast_token(void *data)
{
  struct webrtc_millicast *service = data;
  return service->token;
}
static const char *webrtc_millicast_room(void *data)
{
  return "1";
}

struct obs_service_info webrtc_millicast_service = {
  .id              = "webrtc_millicast",
  .get_name        = webrtc_millicast_name,
  .create          = webrtc_millicast_create,
  .destroy         = webrtc_millicast_destroy,
  .update          = webrtc_millicast_update,
  .get_properties  = webrtc_millicast_properties,
  .get_url         = webrtc_millicast_url,
  .get_milli_id    = webrtc_millicast_id,
  .get_codec       = webrtc_millicast_codec,
  .get_milli_token = webrtc_millicast_token,
  .get_room        = webrtc_millicast_room
};
