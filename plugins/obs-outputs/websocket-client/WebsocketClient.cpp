/* Copyright Dr. Alex. Gouaillard (2015, 2020) */

#include <obs-module.h>
#include <openssl/opensslv.h>
#include "MillicastWebsocketClientImpl.h"
#include "CustomWebrtcImpl.h"

OBS_DECLARE_MODULE()

bool obs_module_load(void)
{
	OPENSSL_init_ssl(0, NULL);
	return true;
}

WEBSOCKETCLIENT_API WebsocketClient *createWebsocketClient(int type)
{
	if (type == Type::Millicast)
		return new MillicastWebsocketClientImpl();
	if (type == Type::CustomWebrtc)
		return new CustomWebrtcImpl();
	return nullptr;
}
