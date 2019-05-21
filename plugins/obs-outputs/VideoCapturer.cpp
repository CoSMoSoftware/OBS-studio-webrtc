#include "VideoCapturer.h"
#include "WebRTCStream.h"

VideoCapturer::VideoCapturer() : captured_frames_(0), start_thread_(nullptr) {}

VideoCapturer::~VideoCapturer() {}

cricket::CaptureState VideoCapturer::Start(const cricket::VideoFormat& capture_format)
{
    std::unique_lock<std::mutex> lock(mutex);

    if (start_thread_) {
        RTC_LOG(LS_ERROR) << "The capturer is already running";
        RTC_DCHECK(start_thread_->IsCurrent())
        << "Trying to start capturer on different threads";
        return cricket::CS_FAILED;
    }

    start_thread_ = rtc::Thread::Current();
    captured_frames_ = 0;
    SetCaptureState(cricket::CS_RUNNING);

    return cricket::CS_STARTING;
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
    SetCaptureState(cricket::CS_STOPPED);
}

void VideoCapturer::OnFrame(const webrtc::VideoFrame& frame)
{
    RTC_DCHECK(start_thread_);
    ++captured_frames_;

    cricket::VideoCapturer::OnFrame(frame, frame.width(), frame.height());
}
