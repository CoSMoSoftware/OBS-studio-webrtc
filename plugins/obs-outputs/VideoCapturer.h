#ifndef _OBS_VIDEO_CAPTURER_
#define _OBS_VIDEO_CAPTURER_
 
#include "media/engine/webrtcvideocapturer.h"

class VideoCapturer : public cricket::WebRtcVideoCapturer
{
public:
  explicit VideoCapturer(cricket::WebRtcVcmFactoryInterface* factory)
  : cricket::WebRtcVideoCapturer(factory) { }
  ~VideoCapturer() { }
  bool Init(const rtc::scoped_refptr<webrtc::VideoCaptureModule>& module);

  void OnFrame(webrtc::VideoFrame const&) {};
};

#endif
