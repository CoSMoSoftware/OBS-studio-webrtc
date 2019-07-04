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
#include "api/media_constraints_interface.h"
#include "api/create_peerconnection_factory.h"
#include "rtc_base/scoped_ref_ptr.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"

#include <regex>
#include <string>

class WebRTCStreamInterface :
  public WebsocketClient::Listener,
  public webrtc::PeerConnectionObserver,
  public webrtc::CreateSessionDescriptionObserver,
  public webrtc::SetSessionDescriptionObserver
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

  //bitrate
  uint64_t getBitrate();

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

  //tracks
  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track;
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track;

  //bitrate and dropped frames
  uint64_t bitrate;
  int dropped_frame;

  //Websocket client
  WebsocketClient* client;

  //Audio Wrapper
  AudioDeviceModuleWrapper adm;

  //Video Capturer
  VideoCapturer* videoCapturer;

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

class SDPModif
{
public:
  enum VideoCodecs {
    VP8,
    VP9,
    H264,
    RED,
    ULPFEC,
    FEC
  };

  static void stereoSDP(std::string &sdp)
  {
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

  static void bitrateSDP(std::string &sdp, int newBitrate)
  {
    std::vector<std::string> sdpLines;
    std::ostringstream newLineBitrate;

    split(sdp, "\r\n", sdpLines);
    int videoLine = findLines(sdpLines, "m=video ");
    newLineBitrate << "b=AS:" << newBitrate;

    sdpLines.insert(sdpLines.begin() + videoLine + 2, newLineBitrate.str());
    sdp = join(sdpLines, "\r\n");
  }

  static void forcePayload(std::string & sdp,
                           std::string video_codec,
                           int packetization_mode = 1,
                           const std::string profile_level_id = "42e01f")
  {
    int line;
    std::ostringstream newLineA;
    std::ostringstream newLineV;
    std::string payloads = "";
    std::vector<std::string> sdpLines;

    split(sdp, (char *)"\r\n", sdpLines);

    processCodec(video_codec, VP8, sdpLines, payloads);
    processCodec(video_codec, VP9, sdpLines, payloads);
    processCodec(video_codec, RED, sdpLines, payloads);
    processCodec(video_codec, ULPFEC, sdpLines, payloads);
    processCodec(video_codec, H264, sdpLines, payloads, packetization_mode, profile_level_id);
  
    line = findLines(sdpLines, "m=video");
    if (line != -1) {
      newLineV << "m=video 9 UDP/TLS/RTP/SAVPF" << payloads;
      sdpLines.insert(sdpLines.begin() + line + 1, newLineV.str());
      sdpLines.erase(sdpLines.begin() + line);
    }

    line = findLines(sdpLines, "m=audio");
    if (line != -1) {
      newLineA << "m=audio 9 UDP/TLS/RTP/SAVPF 111";
      sdpLines.insert(sdpLines.begin() + line + 1, newLineA.str());
      sdpLines.erase(sdpLines.begin() + line);
    }

    do {
      line = findLines(sdpLines, "ISAC/");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);

    do {
      line = findLines(sdpLines, "G722/");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);

    do {
      line = findLines(sdpLines, "ILBC/");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);

    do {
      line = findLines(sdpLines, "PCMU/");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);

    do {
      line = findLines(sdpLines, "PCMA/");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);

    do {
      line = findLines(sdpLines, "CN/");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);

    do {
      line = findLines(sdpLines, "telephone-event/");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);

    sdp = join(sdpLines, "\r\n");
  }

  static void processCodec(std::string & desired_codec, VideoCodecs videoCodec,
                           std::vector<std::string> & sdpLines,
                           std::string & payloads,
                           int desired_packetization_mode,
                           const std::string & desired_profile_level_id)
  {
    int found;
    bool keep = false;
    std::smatch match, matchApt;
    std::string codec, payload, payloadNumber, rtxPayloadNumber;
    std::string h264fmtp = " level-asymmetry-allowed=[0-1];packetization-mode=([0-9]);profile-level-id=([0-9a-f]{6})";

    switch (videoCodec) {
    case VP8:
      codec = "vp8";
      payload = "VP8/90000";
      break;
    case VP9:
      codec = "vp9";
      payload = "VP9/90000";
      break;
    case H264:
      codec = "h264";
      payload = "H264/90000";
      break;
    case RED:
      codec = "red";
      payload = "red/90000";
      break;
    case ULPFEC:
      codec = "ulpfec";
      payload = "ulpfec/90000";
      break;
    case FEC:
      codec = "ulpfec";
      payload = "ulpfec/90000";
      break;
    default:
      codec = "h264";
      payload = "H264/90000";
    }

    do {
      found = findLines(sdpLines, payload);
      if (found != -1) {
        std::string payloadRe = "a=rtpmap:([0-9]+) " + payload;
        std::regex re(payloadRe);
        if (std::regex_search(sdpLines[found], match, re)) {
          payloadNumber = match[1].str();
          if (codec == "h264") {
            int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + h264fmtp);
            if (fmtpLine != -1) {
              std::regex reFmtp(payloadNumber + h264fmtp);
              if (std::regex_search(sdpLines[fmtpLine], match, reFmtp)) {
                int packetization_mode = std::stoi(match[1].str());
                std::string profile_level_id = match[2].str();
                if (desired_codec == codec
                    && desired_packetization_mode == packetization_mode
                    && desired_profile_level_id == profile_level_id) {
                  keep = true;
                } else {
                  keep = false;
                }
              }
            }
          } else {
            if (desired_codec == codec)
              keep = true;
            else
              keep = false;
          }

          if (keep) {
            payloads += " " + payloadNumber;
          } else  {
            int line;
            do {
              line = findLines(sdpLines, ":" + payloadNumber + " ");
              if (line != -1)
                sdpLines.erase(sdpLines.begin() + line);
            } while (line != -1);
          }

          std::string apt = "apt=" + payloadNumber;
          int aptLine = findLines(sdpLines, apt);
          if (aptLine != -1) {
            std::regex reApt("a=fmtp:([0-9]+) apt");
            if (std::regex_search(sdpLines[aptLine], matchApt, reApt)) {
              rtxPayloadNumber = matchApt[1].str();
              if (keep) {
                payloads += " " + rtxPayloadNumber;
              } else {
                sdpLines.erase(sdpLines.begin() + aptLine);
                sdpLines.erase(sdpLines.begin() + aptLine - 1);
              }
            }
          }
        }
      }
    } while (found != -1 && !keep);
  }

  static void processCodec(std::string & desired_codec, VideoCodecs videoCodec,
                           std::vector<std::string> & sdpLines,
                           std::string & payloads)
  {
    return processCodec(desired_codec, videoCodec, sdpLines, payloads, -1, "");
  }

  static std::string join(std::vector<std::string>& v, std::string delim)
  {
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

  static void split(const std::string &s, char* delim, std::vector<std::string> & v)
  {
    char * dup = strdup(s.c_str());
    char * token = strtok(dup, delim);
    while(token != NULL){
      v.push_back(std::string(token));
      token = strtok(NULL, delim);
    }
    free(dup);
  }

  static int findLines(const std::vector<std::string> &sdpLines, std::string prefix)
  {
    for (unsigned long i = 0 ; i < sdpLines.size() ; i++) {
      if ((sdpLines[i].find(prefix) != std::string::npos)) {
        return i;
      }
    }
    return -1;
  }

  static int findLinesRegEx(const std::vector<std::string> &sdpLines, std::string prefix)
  {
    std::regex re(prefix);
    for (unsigned long i = 0; i < sdpLines.size(); i++) {
      std::smatch match;
      if (std::regex_search(sdpLines[i], match, re))
        return i;
    }
    return -1;
  }
};

#endif
