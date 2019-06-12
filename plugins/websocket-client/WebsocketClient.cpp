#include <obs-module.h>
#include <openssl/opensslv.h>
#include "JanusWebsocketClientImpl.h"
#include "EvercastWebsocketClientImpl.h"
#include "MillicastWebsocketClientImpl.h"

OBS_DECLARE_MODULE()

bool obs_module_load(void)
{
  OPENSSL_init_ssl(0, NULL);
  return true;
}

WEBSOCKETCLIENT_API WebsocketClient* createWebsocketClient(int type)
{
  if (type == Type::Janus)
    return new JanusWebsocketClientImpl();
  if (type == Type::Millicast)
    return new MillicastWebsocketClientImpl();
  if (type == Type::Evercast)
    return new EvercastWebsocketClientImpl();

  // error message please

  return nullptr;
}
