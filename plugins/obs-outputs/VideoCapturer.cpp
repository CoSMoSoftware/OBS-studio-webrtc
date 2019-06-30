#include "VideoCapturer.h"
#include "WebRTCStream.h"

VideoCapturer::VideoCapturer() : captured_frames_(0), start_thread_(nullptr) {}

VideoCapturer::~VideoCapturer() {}

webrtc::MediaSourceInterface::SourceState
VideoCapturer::
Start()
{
    std::unique_lock<std::mutex> lock(mutex);

    if (start_thread_) {
        RTC_LOG(LS_ERROR) << "The capturer is already running";
        RTC_DCHECK(start_thread_->IsCurrent())
        << "Trying to start capturer on different threads";
        return webrtc::MediaSourceInterface::kLive;
    }

    start_thread_ = rtc::Thread::Current();
    captured_frames_ = 0;
    state_ = webrtc::MediaSourceInterface::kLive;

    return webrtc::MediaSourceInterface::kInitializing;
}

void VideoCapturer::Stop()
{
    mutex.lock();

    if (!start_thread_) {
        RTC_LOG(LS_ERROR) << "The capturer is already stopped";
        return;
    }

    RTC_DCHECK(start_thread_);
    RTC_DCHECK(start_thread_->IsCurrent());
    mutex.unlock();
    start_thread_ = nullptr;
    state_ = webrtc::MediaSourceInterface::kEnded;
}

void VideoCapturer::OnFrame(const webrtc::VideoFrame& frame)
{
    RTC_DCHECK(start_thread_);
    ++captured_frames_;

    rtc::AdaptedVideoTrackSource::OnFrame(frame);
}

webrtc::MediaSourceInterface::SourceState
VideoCapturer::state() const
{
    return state_;
}
