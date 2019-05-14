#ifndef _OBS_VIDEO_CAPTURER_
#define _OBS_VIDEO_CAPTURER_


#include "media/base/video_capturer.h"
#include "rtc_base/thread.h"

class WebRTCStream;

class VideoCapturer : public cricket::VideoCapturer
{
public:
    VideoCapturer();
    ~VideoCapturer();
    void OnFrame(const webrtc::VideoFrame& frame);

    // video capturer interface
    cricket::CaptureState Start(const cricket::VideoFormat& capture_format) override;
    void Stop() override;
    bool IsRunning() override { return start_thread_; }
    bool IsScreencast() const override { return false; }
    bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) override { return true; }

private:
    rtc::Thread* start_thread_;
    int captured_frames_;
};

#endif
