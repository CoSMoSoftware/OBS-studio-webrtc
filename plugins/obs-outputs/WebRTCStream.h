#ifndef _WEBRTCSTREAM_H_
#define _WEBRTCSTREAM_H_

#pragma comment(lib,"Strmiids.lib") 
#pragma comment(lib,"Secur32.lib")
#pragma comment(lib,"Msdmo.lib")
#pragma comment(lib,"dmoguids.lib")
#pragma comment(lib,"wmcodecdspuuid.lib")
#pragma comment(lib,"amstrmid.lib")

#include "obs.h"
#include "WebsocketClient.h"
#include "VideoCapture.h"
#include "VideoCapturer.h"
#include "AudioDeviceModuleWrapper.h"

#include "api/mediastreaminterface.h"
#include "api/peerconnectioninterface.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_defines.h"
#include "modules/video_capture/video_capture_factory.h"
#include "api/mediaconstraintsinterface.h"
#include "api/peerconnectioninterface.h"
#include "rtc_base/scoped_ref_ptr.h"
#include "rtc_base/refcountedobject.h"
#include "rtc_base/thread.h"

class WebRTCStreamInterface :
  public WebsocketClient::Listener,
  public webrtc::PeerConnectionObserver,
  public webrtc::CreateSessionDescriptionObserver,
  public webrtc::SetSessionDescriptionObserver,
  public cricket::WebRtcVcmFactoryInterface
{

};

class WebRTCStream : public rtc::RefCountedObject<WebRTCStreamInterface>
{
public:
  WebRTCStream(obs_output_t *output);
  ~WebRTCStream();

  bool start();
  void onVideoFrame(video_data *frame);
  void onAudioFrame(audio_data *frame);
  bool stop();

  //
  // WebsocketClient::Listener implementation.
  //
  virtual void onConnected();
  virtual void onLogged(int code);
  virtual void onLoggedError(int code);
  virtual void onOpened(const std::string &sdp);
  virtual void onOpenedError(int code);
  virtual void onDisconnected();

  //
  // PeerConnectionObserver implementation.
  //
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {};
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {};
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {};
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {};
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {};
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(const std::string& error) override;

  // SetSessionDescriptionObserver implementation
  void OnSuccess() override;
  //void OnFailure(const std::string& error) override;

  virtual rtc::scoped_refptr<webrtc::VideoCaptureModule> Create(const char* device)
  {
    return videoCapture;
  }
  virtual webrtc::VideoCaptureModule::DeviceInfo* CreateDeviceInfo()
  {
    return webrtc::VideoCaptureFactory::CreateDeviceInfo();
  }
  virtual void DestroyDeviceInfo(webrtc::VideoCaptureModule::DeviceInfo* info)
  {
    delete(info);
  }

private:
  //Connection properties
  std::string url;
  long long room;
  std::string username;
  std::string password;
  //Websocket client
  WebsocketClient* client;
  //Audio Wrapper
  AudioDeviceModuleWrapper adm;
  //Video Wrappers
  webrtc::VideoCaptureCapability videoCaptureCapability;
  rtc::scoped_refptr<VideoCapture> videoCapture;
  //Peerconnection
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;
  //WebRTC threads
  std::unique_ptr<rtc::Thread> network;
  std::unique_ptr<rtc::Thread> worker;
  std::unique_ptr<rtc::Thread> signaling;
  //OBS stream output
  obs_output_t *output;
};

#endif
