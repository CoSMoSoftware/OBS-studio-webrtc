#include "VideoCapturer.h"

/* Based on the peerconnection example. Might work with Windows/Linux */

rtc::scoped_refptr<VideoCapturer> VideoCapturer::Create(size_t Width, size_t Height) {

    const size_t Fps = 30;
    const size_t DeviceIndex = 0;
    std::unique_ptr<webrtc::test::VcmCapturer> capturer = absl::WrapUnique(webrtc::test::VcmCapturer::Create(Width, Height, Fps, DeviceIndex));
    if (!capturer) {
        return nullptr;
    }
    rtc::scoped_refptr<VideoCapturer> Vc = new rtc::RefCountedObject<VideoCapturer>(std::move(capturer));
    return Vc;
}


/* VERSION 65
bool VideoCapturer::Init(const rtc::scoped_refptr<webrtc::VideoCaptureModule>& module)
{
  //Call parent
  if (!cricket::WebRtcVideoCapturer::Init(module))
    return false;
  //Set supported formats
  std::vector<cricket::VideoFormat> supported;
  //Push them
  supported.push_back(cricket::VideoFormat(600, 400, 30, cricket::FOURCC_NV12));
  //Set them
  SetSupportedFormats(supported);
  //OK
  return true;
}
*/
