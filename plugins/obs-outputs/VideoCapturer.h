/* Copyright Dr. Alex. Gouaillard (2015, 2020) */

#ifndef _OBS_VIDEO_CAPTURER_
#define _OBS_VIDEO_CAPTURER_

#include <media/base/adapted_video_track_source.h>
#include <rtc_base/thread.h>

#include <mutex>

class VideoCapturer : public rtc::AdaptedVideoTrackSource {
public:
	VideoCapturer();
	~VideoCapturer() override;

	void OnFrameCaptured(const webrtc::VideoFrame &frame);

	// VideoTrackSourceInterface API
	bool is_screencast() const override { return false; }
	absl::optional<bool> needs_denoising() const override { return false; }

	// MediaSourceInterface API
	webrtc::MediaSourceInterface::SourceState state() const override
	{
		return webrtc::MediaSourceInterface::SourceState::kLive;
	}
	bool remote() const override { return false; }

private:
	rtc::Thread *start_thread_;
	std::mutex mutex;
};

#endif
