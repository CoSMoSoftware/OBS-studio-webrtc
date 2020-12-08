/* Copyright Dr. Alex. Gouaillard (2015, 2020) */

#include "VideoCapturer.h"

VideoCapturer::VideoCapturer() {}

VideoCapturer::~VideoCapturer() = default;

void VideoCapturer::OnFrameCaptured(const webrtc::VideoFrame &frame)
{
	rtc::AdaptedVideoTrackSource::OnFrame(frame);
}
