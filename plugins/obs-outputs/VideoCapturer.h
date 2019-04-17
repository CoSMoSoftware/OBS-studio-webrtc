#ifndef _OBS_VIDEO_CAPTURER_
#define _OBS_VIDEO_CAPTURER_

/* Based on the peerconnection example. Might work with Windows/Linux */

#include "pc/video_track_source.h"
#include "test/vcm_capturer.h"


class VideoCapturer : public webrtc::VideoTrackSource
{
public:
    ~VideoCapturer() { }
    static rtc::scoped_refptr<VideoCapturer> Create(size_t, size_t);

protected:
    explicit VideoCapturer(std::unique_ptr<webrtc::test::VcmCapturer> capturer)
    : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}
    
private:
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
        return capturer_.get();
    }
    std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};


/* VERSION 65
#include "media/engine/webrtcvideocapturer.h"

class VideoCapturer : public cricket::WebRtcVideoCapturer
{
public:
  explicit VideoCapturer(cricket::WebRtcVcmFactoryInterface* factory)
  : cricket::WebRtcVideoCapturer(factory) { }
  ~VideoCapturer() { }
  bool Init(const rtc::scoped_refptr<webrtc::VideoCaptureModule>& module);
};
*/

#endif
