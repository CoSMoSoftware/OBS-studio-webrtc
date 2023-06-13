#include "useless_cc_network_controller.h"

namespace webrtc {

NetworkControlUpdate UselessCcNetworkController::OnNetworkAvailability(
    NetworkAvailability msg) {
  NetworkControlUpdate update;
  return update;
}

NetworkControlUpdate UselessCcNetworkController::OnNetworkRouteChange(
    NetworkRouteChange msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate UselessCcNetworkController::OnProcessInterval(
    ProcessInterval msg) {
  NetworkControlUpdate update;
  update.target_rate = TargetTransferRate();
  if (max_data_rate_.IsFinite()) {
    update.target_rate->target_rate = max_data_rate_;
    update.target_rate->stable_target_rate = max_data_rate_;
  }
  update.target_rate->network_estimate.at_time = msg.at_time;
  update.target_rate->network_estimate.round_trip_time = round_trip_time_;
  update.target_rate->network_estimate.bwe_period = TimeDelta::Zero();

  update.target_rate->at_time = msg.at_time;
  return update;
}

NetworkControlUpdate UselessCcNetworkController::OnRemoteBitrateReport(
    RemoteBitrateReport msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate UselessCcNetworkController::OnRoundTripTimeUpdate(
    RoundTripTimeUpdate msg) {
  if (!msg.round_trip_time.IsZero()) {
    round_trip_time_ = msg.round_trip_time;
  }

  return NetworkControlUpdate();
}

NetworkControlUpdate UselessCcNetworkController::OnSentPacket(
    SentPacket sent_packet) {
  return NetworkControlUpdate();
}

NetworkControlUpdate UselessCcNetworkController::OnReceivedPacket(
    ReceivedPacket received_packet) {
  return NetworkControlUpdate();
}

NetworkControlUpdate UselessCcNetworkController::OnStreamsConfig(
    StreamsConfig msg) {
  NetworkControlUpdate update;
  return update;
}

NetworkControlUpdate UselessCcNetworkController::OnTargetRateConstraints(
    TargetRateConstraints constraints) {
  if (constraints.max_data_rate.has_value()) {
    max_data_rate_ = constraints.max_data_rate.value();
  }
  NetworkControlUpdate update;
  update.target_rate = TargetTransferRate();
  update.target_rate->target_rate = max_data_rate_;
  update.target_rate->stable_target_rate = max_data_rate_;
  return update;
}

NetworkControlUpdate UselessCcNetworkController::OnTransportLossReport(
    TransportLossReport msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate UselessCcNetworkController::OnTransportPacketsFeedback(
    TransportPacketsFeedback msg) {
  return NetworkControlUpdate();
}

NetworkControlUpdate UselessCcNetworkController::OnNetworkStateEstimate(
    NetworkStateEstimate msg) {
  return NetworkControlUpdate();
}

PacerConfig UselessCcNetworkController::GetPacingRates(Timestamp at_time) const {
  // Pacing rate is based on target rate before congestion window pushback,
  // because we don't want to build queues in the pacer when pushback occurs.
  DataRate pacing_rate = DataRate::Zero();
  pacing_rate = max_data_rate_ * pacing_factor_;
  DataRate padding_rate = max_padding_rate_;
  PacerConfig msg;
  msg.at_time = at_time;
  msg.time_window = TimeDelta::Seconds(1);
  msg.data_window = pacing_rate * msg.time_window;
  msg.pad_window = padding_rate * msg.time_window;
  return msg;
}

};  // namespace webrtc
