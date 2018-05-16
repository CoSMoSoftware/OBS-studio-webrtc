#include <../../libobs/obs-module.h>
#include <openssl/opensslv.h>
#include "JanusWebsocketClientImpl.h"
#include "SpankChainWebsocketClientImpl.h"

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
  if (type == WEBSOCKETCLIENT_SPANKCHAIN)
    return new SpankChainWebsocketClientImpl();
  return nullptr;
}
