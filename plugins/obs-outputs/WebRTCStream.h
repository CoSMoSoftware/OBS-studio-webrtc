#ifndef _WEBRTCSTREAM_H_
#define _WEBRTCSTREAM_H_

// NOTE ALEX: WTF
#if WIN32
#pragma comment(lib,"Strmiids.lib")
#pragma comment(lib,"Secur32.lib")
#pragma comment(lib,"Msdmo.lib")
#pragma comment(lib,"dmoguids.lib")
#pragma comment(lib,"wmcodecdspuuid.lib")
#pragma comment(lib,"amstrmid.lib")
#endif

#include "obs.h"
#include "WebsocketClient.h"
#include "VideoCapturer.h"
#include "AudioDeviceModuleWrapper.h"

#include <rtc_base/platform_file.h>
#include <rtc_base/bitrate_allocation_strategy.h>
#include <modules/audio_processing/include/audio_processing.h>

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "api/create_peerconnection_factory.h"
#include "api/scoped_refptr.h"
#include "api/set_remote_description_observer_interface.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "rtc_base/timestamp_aligner.h"

class WebRTCStreamInterface :
  public WebsocketClient::Listener,
  public webrtc::PeerConnectionObserver,
  public webrtc::CreateSessionDescriptionObserver,
  public webrtc::SetSessionDescriptionObserver,
  public webrtc::SetRemoteDescriptionObserverInterface
{

};
class WebRTCStream : public rtc::RefCountedObject<WebRTCStreamInterface>
{
public:
  enum Type {
    Janus      = 0,
    SpankChain = 1,
    Millicast  = 2,
    Evercast   = 3
  };
public:
  WebRTCStream(obs_output_t *output);
  ~WebRTCStream();

  bool start(Type type);
  void onVideoFrame(video_data *frame);
  void onAudioFrame(audio_data *frame);
  bool enableThumbnail(uint8_t downscale, uint8_t downrate)
  {
    if (!downscale || !downrate)
      return false;
    thumbnailDownscale = downscale;
    thumbnailDownrate = downrate;
    return (thumbnail = true);
  }
  void setCodec(const std::string& codec)
  {
    this->codec = codec;
  }
  bool stop();

  //
  // WebsocketClient::Listener implementation.
  //
  virtual void onConnected(    ) override;
  virtual void onDisconnected( ) override;
  virtual void onLogged(      int code ) override;
  virtual void onLoggedError( int code ) override;
  virtual void onOpenedError( int code ) override;
  virtual void onOpened( const std::string &sdp ) override;

  //
  // PeerConnectionObserver implementation.
  //
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange( webrtc::PeerConnectionInterface::IceConnectionState /*new_state*/) override {};
  void OnIceGatheringChange(  webrtc::PeerConnectionInterface::IceGatheringState  /*new_state*/) override {};
  void OnSignalingChange(     webrtc::PeerConnectionInterface::SignalingState     /*new_state*/) override {};
  void OnAddStream(   rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) override {};
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) override {};
  void OnDataChannel( rtc::scoped_refptr<webrtc::DataChannelInterface> /*channel*/) override {}
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool /*receiving*/) override {}

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(const std::string& error) override;
  // SetSessionDescriptionObserver implementation
  void OnSuccess() override;
  //void OnFailure(const std::string& error) override;

  // SetRemoteDescriptionObserverInterface implementation
  void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

  //bitrate
  uint64_t getBitrate();

  template <typename T>
  rtc::scoped_refptr<T> make_scoped_refptr(T* t) {
    return rtc::scoped_refptr<T>(t);
  }

private:
  //Connection properties
  std::string url;
  std::string room;
  std::string username;
  std::string password;
  std::string codec;
  std::string milliId;
  std::string milliToken;
  bool        thumbnail;

  //SetRemoteDescription Observer
  rtc::scoped_refptr<SetRemoteDescriptionObserverInterface> srdoi_observer;

  //tracks
  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track;
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track;

  //bitrate and dropped frames
  uint64_t bitrate;
  int dropped_frame;
  uint16_t id;

  //Websocket client
  WebsocketClient* client;

  //Audio Wrapper
  AudioDeviceModuleWrapper adm;

  //Video Capturer
  rtc::scoped_refptr<VideoCapturer> videoCapturer;
  rtc::TimestampAligner timestamp_aligner_;

  //Thumbnail wrapper
  //webrtc::VideoCaptureCapability thumbnailCaptureCapability;
  //rtc::scoped_refptr<VideoCapture> thumbnailCapture;
  uint32_t picId;
  uint8_t thumbnailDownscale;
  uint8_t thumbnailDownrate;

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

class SDPModif {
public:
  static void stereoSDP(std::string &sdp) {
    std::vector<std::string> sdpLines;
    split(sdp, "\r\n", sdpLines);
    int fmtpLine = findLines(sdpLines, "a=fmtp:111");
    if (fmtpLine != -1) {
      sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";stereo=1;sprop-stereo=1");
    } else {
      int artpLine = findLines(sdpLines, "a=rtpmap:111 opus/48000/2");
      sdpLines.insert(sdpLines.begin() + artpLine + 1, "a=fmtp:111 stereo=1;sprop-stereo=1");
    }
    sdp = join(sdpLines, "\r\n");
  }

  static void bitrateSDP(std::string &sdp, int newBitrate) {
    std::vector<std::string> sdpLines;
    std::ostringstream newLineBitrate;

    split(sdp, "\r\n", sdpLines);
    int videoLine = findLines(sdpLines, "m=video ");
    newLineBitrate << "b=AS:" << newBitrate;

    sdpLines.insert(sdpLines.begin() + videoLine + 2, newLineBitrate.str());
    sdp = join(sdpLines, "\r\n");
  }

  static std::string join(std::vector<std::string>& v, std::string delim) {
    std::ostringstream s;
    for (const auto& i : v) {
      if (&i != &v[0]) {
          s << delim ;
      }
      s << i;
    }
    s << delim;
    return s.str();
  }

  static void split(const std::string &s, char* delim, std::vector<std::string> & v){
    char * dup = strdup(s.c_str());
    char * token = strtok(dup, delim);
    while(token != NULL){
      v.push_back(std::string(token));
      token = strtok(NULL, delim);
    }
    free(dup);
  }

  static int findLines(std::vector<std::string> sdpLines, std::string prefix) {
    for (unsigned long i = 0 ; i < sdpLines.size() ; i++) {
      if ((sdpLines[i].find(prefix) != std::string::npos)) {
        return i;
      }
    }
    return -1;
  }
};

#endif
