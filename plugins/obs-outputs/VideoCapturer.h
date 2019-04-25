#ifndef _OBS_VIDEO_CAPTURER_
#define _OBS_VIDEO_CAPTURER_

 
#include "media/base/video_capturer.h"
#include "modules/video_capture/video_capture.h"

#include "WebRTCStream.h"

class VideoCapturer  : public cricket::VideoCapturer // : public cricket::WebRtcVideoCapturer
{
public:
//  explicit VideoCapturer(cricket::WebRtcVcmFactoryInterface* factory)
// : cricket::WebRtcVideoCapturer(factory) { }

//  VideoCapturer(WebRTCStream *) { };


  // video capture interface
  virtual cricket::CaptureState Start(const cricket::VideoFormat& capture_format) override {};
  virtual void Stop() override {};
  virtual bool IsRunning() override {};
  virtual bool IsScreencast() const override {};
  virtual bool GetPreferredFourccs(std::vector<uint32_t>* fourccs) override {};

  ~VideoCapturer() { }
  bool Init(const rtc::scoped_refptr<webrtc::VideoCaptureModule>& module);
};

#endif
