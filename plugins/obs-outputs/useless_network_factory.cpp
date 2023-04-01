/// Custom network factory for injection

#include "useless_network_factory.h"
#include "useless_cc_network_controller.h"
#include "rtc_base/logging.h"

namespace webrtc
{
  // NetworkControllerFactoryInterface is an interface for creating a network
  // controller.

  // Used to create a new network controller, requires an observer to be
  // provided to handle callbacks.
  std::unique_ptr<NetworkControllerInterface> UselessNetworkControllerFactory::Create(
      NetworkControllerConfig config)
  {
      RTC_LOG(LS_INFO) << "Disabling Google CC Bandwidth Estimation.";
      return std::make_unique<UselessCcNetworkController>(config);
  }
  // Returns the interval by which the network controller expects
  // OnProcessInterval calls.
  TimeDelta UselessNetworkControllerFactory::GetProcessInterval() const
  {
    const int64_t kUpdateIntervalMs = 25;
    return TimeDelta::Millis(kUpdateIntervalMs);
  };
}