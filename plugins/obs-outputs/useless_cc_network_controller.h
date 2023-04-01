#ifndef _USELESS_CC_NETWORK_CONTROLLER_H_
#define _USELESS_CC_NETWORK_CONTROLLER_H_

/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_CONGESTION_CONTROLLER_USELESS_CC_NETWORK_CONTROLLER_H_
#define MODULES_CONGESTION_CONTROLLER_USELESS_CC_NETWORK_CONTROLLER_H_
#endif

#include "absl/types/optional.h"
#include "api/transport/network_control.h"
#include "api/transport/network_types.h"
#include "api/units/data_rate.h"
#include "api/units/data_size.h"
#include "api/units/timestamp.h"

namespace webrtc {

class UselessCcNetworkController : public NetworkControllerInterface {
 public:

  UselessCcNetworkController(webrtc::NetworkControllerConfig config)
  {
    if (config.constraints.max_data_rate.has_value()) {
      max_data_rate_ = config.constraints.max_data_rate.value();
    }
  }

  // NetworkControllerInterface
  NetworkControlUpdate OnNetworkAvailability(NetworkAvailability msg) override;
  NetworkControlUpdate OnNetworkRouteChange(NetworkRouteChange msg) override;
  NetworkControlUpdate OnProcessInterval(ProcessInterval msg) override;
  NetworkControlUpdate OnRemoteBitrateReport(RemoteBitrateReport msg) override;
  NetworkControlUpdate OnRoundTripTimeUpdate(RoundTripTimeUpdate msg) override;
  NetworkControlUpdate OnSentPacket(SentPacket msg) override;
  NetworkControlUpdate OnReceivedPacket(ReceivedPacket msg) override;
  NetworkControlUpdate OnStreamsConfig(StreamsConfig msg) override;
  NetworkControlUpdate OnTargetRateConstraints(
      TargetRateConstraints msg) override;
  NetworkControlUpdate OnTransportLossReport(TransportLossReport msg) override;
  NetworkControlUpdate OnTransportPacketsFeedback(
      TransportPacketsFeedback msg) override;
  NetworkControlUpdate OnNetworkStateEstimate(
      NetworkStateEstimate msg) override;

  NetworkControlUpdate GetNetworkState(Timestamp at_time) const;
 private:
  PacerConfig GetPacingRates(Timestamp at_time) const;
  DataRate max_data_rate_ = DataRate::PlusInfinity();
  DataRate max_total_allocated_bitrate_ = DataRate::PlusInfinity();
  DataRate min_total_allocated_bitrate_ = DataRate::PlusInfinity();
  DataRate max_padding_rate_ = DataRate::PlusInfinity();
  TimeDelta round_trip_time_ = TimeDelta::Zero();

  double pacing_factor_{0.0f};
};

};  // namespace webrtc

#endif // _USELESS_CC_NETWORK_CONTROLLER_H_
