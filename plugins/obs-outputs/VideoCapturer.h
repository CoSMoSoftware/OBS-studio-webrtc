#ifndef _OBS_VIDEO_CAPTURER_
#define _OBS_VIDEO_CAPTURER_

#include "media/base/adapted_video_track_source.h"
#include "rtc_base/thread.h"

#include <mutex>

class WebRTCStream;

class VideoCapturer : public rtc::AdaptedVideoTrackSource
{
public:
    VideoCapturer();
    ~VideoCapturer();
    void OnFrame(const webrtc::VideoFrame& frame);

    webrtc::MediaSourceInterface::SourceState Start();
    void Stop();
    bool IsRunning() { return start_thread_; }
    bool IsScreencast() const { return false; }
    bool GetPreferredFourccs(std::vector<uint32_t> * /* unused fourccs */) { return true; }

    // VideoTrackSourceInterface API
    bool is_screencast() const override { return false; }
    absl::optional<bool> needs_denoising() const override { return false; }

    // MediaSourceInterface API
    webrtc::MediaSourceInterface::SourceState state() const override;
    bool remote() const override { return false; }

private:
    rtc::Thread* start_thread_;
    int captured_frames_;
    std::mutex mutex;
    webrtc::MediaSourceInterface::SourceState state_;
};

#endif
