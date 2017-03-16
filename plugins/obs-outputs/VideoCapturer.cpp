#include "VideoCapturer.h"


bool  VideoCapturer::Init(const rtc::scoped_refptr<webrtc::VideoCaptureModule>& module)
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