#include <obs-module.h>
#include <openssl/opensslv.h>
#include "JanusWebsocketClientImpl.h"
#include "MillicastWebsocketClientImpl.h"

OBS_DECLARE_MODULE()

bool obs_module_load(void)
{
  OPENSSL_init_ssl(0, NULL);
  return true;
}

WEBSOCKETCLIENT_API WebsocketClient* createWebsocketClient(int type)
{
  if (type == WEBSOCKETCLIENT_JANUS)
    return new JanusWebsocketClientImpl();
  if (type == WEBSOCKETCLIENT_MILLICAST)
    return new MillicastWebsocketClientImpl();
  return nullptr;
}
