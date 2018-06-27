#include "api/stats/rtcstatscollectorcallback.h"

class RTCStatsCollectorCallbackBridge : public webrtc::RTCStatsCollectorCallback
{
private:
  std::string report_;
  mutable webrtc::webrtc_impl::RefCounter ref_count_{ 0 };

public:
  std::string getReport() { return report_; }

  void OnStatsDelivered(
    const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override{
      report_ = report->ToJson();
  }

  void AddRef() const override {
    ref_count_.IncRef();
  }

  rtc::RefCountReleaseStatus Release() const override {
    const auto status = ref_count_.DecRef();
    if (status == rtc::RefCountReleaseStatus::kDroppedLastRef) {
      delete this;
    }
    return status;
  } 
};
