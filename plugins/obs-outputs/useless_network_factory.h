#ifndef _USELESS_NETWORK_FACTORY_H_
#define _USELESS_NETWORK_FACTORY_H_

/// Custom network factory for injection

#include "api/transport/network_control.h"

namespace webrtc {
class UselessNetworkControllerFactory
	: public NetworkControllerFactoryInterface {
public:
	virtual ~UselessNetworkControllerFactory() = default;

	// Used to create a new network controller, requires an observer to be
	// provided to handle callbacks.
	virtual std::unique_ptr<NetworkControllerInterface>
	Create(NetworkControllerConfig config) override;
	// Returns the interval by which the network controller expects
	// OnProcessInterval calls.
	virtual TimeDelta GetProcessInterval() const override;
};
}

#endif // _USELESS_NETWORK_FACTORY_H_
