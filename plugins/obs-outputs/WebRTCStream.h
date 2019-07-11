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
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "rtc_base/timestamp_aligner.h"

#include <initializer_list>
#include <regex>
#include <string>
#include <vector>

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
    Wowza      = 1,
    Millicast  = 2,
    Evercast   = 3,
    SpankChain = 4
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
  virtual void onRemoteIceCandidate(const std::string& sdpData) override;

  //
  // PeerConnectionObserver implementation.
  //
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange( webrtc::PeerConnectionInterface::IceConnectionState /*new_state*/) override {}
  void OnIceGatheringChange(  webrtc::PeerConnectionInterface::IceGatheringState  /*new_state*/) override {}
  void OnSignalingChange(     webrtc::PeerConnectionInterface::SignalingState     /*new_state*/) override {}
  void OnAddStream(   rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) override {}
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> /*stream*/) override {}
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
  Type type;
  std::string url;
  std::string room;
  std::string username;
  std::string password;
  std::string codec;
  std::string milliId;
  std::string milliToken;
  std::string protocol;
  bool        thumbnail;

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
  enum VideoCodecs {
    VP8,
    VP9,
    H264,
    RED,
    ULPFEC,
    FEC
  };

  static void stereoSDP(std::string &sdp, int audioBitrate)
  {
    std::string aBitrate = std::to_string(audioBitrate);
    std::string maxAvgBitrate = std::to_string(audioBitrate * 1024);
    std::vector<std::string> sdpLines;
    split(sdp, (char*)"\r\n", sdpLines);
    int testLine = findLines(sdpLines, "stereo=1;sprop-stereo=1");
    if (testLine == -1) {
      int fmtpLine = findLines(sdpLines, "a=fmtp:111");
      if (fmtpLine != -1) {
        sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";stereo=1;sprop-stereo=1");
        if (audioBitrate > 0) {
          sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";maxaveragebitrate=" + maxAvgBitrate);
          sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";x-google-min-bitrate=" + aBitrate);
          sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";x-google-max-bitrate=" + aBitrate);
        }
      } else {
        int artpLine = findLines(sdpLines, "a=rtpmap:111 opus/48000/2");
        if (artpLine != -1) {
          sdpLines.insert(sdpLines.begin() + artpLine + 1, "a=fmtp:111 stereo=1;sprop-stereo=1");
          if (audioBitrate > 0) {
            sdpLines[artpLine+1] = sdpLines[artpLine+1].append(";maxaveragebitrate=" + maxAvgBitrate);
            sdpLines[artpLine+1] = sdpLines[artpLine+1].append(";x-google-min-bitrate=" + aBitrate);
            sdpLines[artpLine+1] = sdpLines[artpLine+1].append(";x-google-max-bitrate=" + aBitrate);
          }
        }
      }
    }
    sdp = join(sdpLines, "\r\n");
  }

  static void bitrateSDP(std::string &sdp, int newBitrate)
  {
    std::string vBitrate = std::to_string(newBitrate);
    std::ostringstream newLineBitrate;
    std::vector<std::string> sdpLines;
    split(sdp, (char*)"\r\n", sdpLines);
    int testLine = findLines(sdpLines, "b=AS:");
    if (testLine == -1) {
      int videoLine = findLines(sdpLines, "m=video ");
      newLineBitrate << "b=AS:" << newBitrate;
      sdpLines.insert(sdpLines.begin() + videoLine + 2, newLineBitrate.str());
    }
    sdp = join(sdpLines, "\r\n");
  }

  static void bitrateMaxMinSDP(
    std::string& sdp,
    const int newBitrate,
    const std::vector<int>& video_payload_numbers
  )
  {
    int fmtpLine = -1;
    int testLine = 0;
    std::string vBitrate = std::to_string(newBitrate);
    std::ostringstream newLineBitrate;
    std::vector<std::string> sdpLines;

    split(sdp, (char *)"\r\n", sdpLines);

    testLine = findLines(sdpLines, "b=AS:");
    if (testLine == -1) {
      int videoLine = findLines(sdpLines, "m=video ");
      newLineBitrate << "b=AS:" << newBitrate;
      sdpLines.insert(sdpLines.begin() + videoLine + 2, newLineBitrate.str());
    }

    testLine = findLines(sdpLines, "x-google-min-bitrate=" + vBitrate);
    if (testLine == -1) {
      for (auto payload : video_payload_numbers) {
        fmtpLine = findLines(sdpLines, "a=fmtp:" + std::to_string(payload));
        if (fmtpLine != -1) {
          sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";x-google-min-bitrate=" + vBitrate);
          sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";x-google-max-bitrate=" + vBitrate);
        } else {
          int vrtpLine = findLines(sdpLines, "a=rtpmap:" + std::to_string(payload));
          if (vrtpLine != -1) {
            std::string newLineFmtp = "a=fmtp:" + std::to_string(payload);
            sdpLines.insert(sdpLines.begin() + vrtpLine + 1, newLineFmtp);
            sdpLines[vrtpLine+1] = sdpLines[vrtpLine+1].append(" x-google-min-bitrate=" + vBitrate);
            sdpLines[vrtpLine+1] = sdpLines[vrtpLine+1].append(";x-google-max-bitrate=" + vBitrate);
          }
        }
      }
    }

    sdp = join(sdpLines, "\r\n");
  }

  static bool filterIceCandidates(const std::string& candidate, const std::string& protocol)
  {
    std::smatch match;
    std::regex re("candidate:([0-9]+) ([0-9]+) (TCP|UDP) ([0-9]+) ([0-9.]+) ([0-9]+) ([^0-9]+) ([0-9]+)");
    if (std::regex_search(candidate, match, re)) {
      if (match[3].str().compare(protocol) == 0)
        return true;
      return false;
    }
    return false;
  }

  static void forcePayload(
    std::string& sdp,
    const std::string& video_codec,
    std::vector<int>& video_payload_numbers
  )
  {
    return forcePayload(sdp, video_codec, video_payload_numbers,
                        0, "42e01f", 0);
  }

  static void forcePayload(
    std::string& sdp,
    const std::string& video_codec,
    std::vector<int>& video_payload_numbers,
    const int vp9_profile_id
  )
  {
    return forcePayload(sdp, video_codec, video_payload_numbers,
                        0, "42e01f", vp9_profile_id);
  }

  static void forcePayload(
    std::string& sdp,
    const std::string& video_codec,
    std::vector<int>& video_payload_numbers,
    const int h264_packetization_mode,
    const std::string& h264_profile_level_id
  )
  {
    return forcePayload(sdp, video_codec, video_payload_numbers,
                        h264_packetization_mode, h264_profile_level_id, 0);
  }

  static void forcePayload(
    std::string& sdp,
    const std::string& video_codec,
    std::vector<int>& video_payload_numbers,
    const int h264_packetization_mode,
    const std::string& h264_profile_level_id,
    const int vp9_profile_id
  )
  {
    std::string audio_codec = "opus";
    int line;
    std::ostringstream newLineA;
    std::ostringstream newLineV;
    std::string payloads = "";
    std::vector<std::string> sdpLines;
 
    filterCodecs(sdp, payloads, video_payload_numbers, video_codec, audio_codec,
                 h264_packetization_mode, h264_profile_level_id, vp9_profile_id);

    split(sdp, (char*)"\r\n", sdpLines);
  
    // if (caseInsensitiveStringCompare(video_codec, "h264"))
    //   processCodec(video_codec, H264, sdpLines, payloads, video_payload_numbers, h264_packetization_mode, h264_profile_level_id);
    // if (caseInsensitiveStringCompare(video_codec, "vp9"))
    //   processCodec(video_codec, VP9, sdpLines, payloads, video_payload_numbers, vp9_profile_id);

    // processCodec(video_codec, VP8, sdpLines, payloads, video_payload_numbers);
    // processCodec(video_codec, VP9, sdpLines, payloads, video_payload_numbers, vp9_profile_id);
    // processCodec(video_codec, RED, sdpLines, payloads, video_payload_numbers);
    // processCodec(video_codec, ULPFEC, sdpLines, payloads, video_payload_numbers);
    // processCodec(video_codec, H264, sdpLines, payloads, video_payload_numbers, h264_packetization_mode, h264_profile_level_id);

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

    /*
    std::vector<std::string> audio_codecs_to_remove =
      { "ISAC", "G722", "ILBC", "PCMU", "PCMA", "CN", "telephone-event" };

    for (const std::string audio_codec : audio_codecs_to_remove) {
      do {
        line = findLines(sdpLines, "rtpmap");
        std::smatch match;
        std::string payloadCodec;
        int payloadNumber;
        const char* payloadRe = "a=rtpmap:([0-9]+) ([a-z0-9-]+)/([0-9]+/?[0-9]*)";
        auto re = std::regex{payloadRe, std::regex_constants::icase};
        if (std::regex_search(sdpLines[line], match, re)) {
          payloadNumber = std::stoi(match[1].str());
          payloadCodec = match[2].str();
        }
        line = findLines(sdpLines, audio_codec + "/");
        if (line != -1)
          sdpLines.erase(sdpLines.begin() + line);
      } while (line != -1);
    }
    */

    sdp = join(sdpLines, "\r\n");
  }

  static void deletePayload(std::string& sdp, int payloadNumber)
  {
    std::vector<std::string> sdpLines;
    split(sdp, (char*)"\r\n", sdpLines);
    int line;
    do {
      line = findLines(sdpLines, ":" + std::to_string(payloadNumber) + " ");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);
    sdp = join(sdpLines, "\r\n");
  }

  static void deletePayload(std::vector<std::string>& sdpLines, int payloadNumber)
  {
    int line;
    do {
      line = findLines(sdpLines, ":" + std::to_string(payloadNumber) + " ");
      if (line != -1)
        sdpLines.erase(sdpLines.begin() + line);
    } while (line != -1);
  }

  // remove all other codecs
  static void filterCodecs(
    std::string& sdp,
    std::string& payloads,
    std::vector<int>& video_payload_numbers,
    const std::string& video_codec
  )
  {
    return filterCodecs(sdp, payloads, video_payload_numbers, video_codec, "opus", 0, "42e01f", 0);
  }

  // remove all other codecs
  static void filterCodecs(
    std::string& sdp,
    std::string& payloads,
    std::vector<int>& video_payload_numbers,
    const std::initializer_list<std::string>& codecs
  )
  {
    return filterCodecs(sdp, payloads, video_payload_numbers, codecs, 0, "42e01f", 0);
  }

  // remove all other codecs
  static void filterCodecs(
    std::string& sdp,
    std::string& payloads,
    std::vector<int>& video_payload_numbers,
    const std::string& video_codec,
    const std::string& audio_codec,
    const int h264_packetization_mode,
    const std::string& h264_profile_level_id,
    const int vp9_profile_id
  )
  {
    std::string h264fmtp = " level-asymmetry-allowed=[0-1];packetization-mode=([0-9]);profile-level-id=([0-9a-f]{6})";
    std::string vp9fmtp = " profile-id=([0-9])";
    std::vector<int> apt_payload_numbers;
    std::vector<std::string> sdpLines;
    split(sdp, (char*)"\r\n", sdpLines);
    int line = -1;
    for (std::string sdpLine : sdpLines) {
      ++line;
      std::string payloadCodec;
      std::string payloadNumber;
      std::string payloadRe = "a=rtpmap:([0-9]+) ([A-Za-z0-9-]+)";
      std::regex re(payloadRe);
      std::smatch match;
      if (std::regex_search(sdpLine, match, re)) {
        payloadNumber = match[1].str();
        payloadCodec = match[2].str();
        bool keep = false;
        bool aptKeep = false;
        if (caseInsensitiveStringCompare(video_codec, payloadCodec)) {
          if (caseInsensitiveStringCompare("h264", payloadCodec)) {
            int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + h264fmtp);
            if (fmtpLine != -1) {
              std::smatch matchFmtp;
              std::regex reFmtp(payloadNumber + h264fmtp);
              if (std::regex_search(sdpLines[fmtpLine], matchFmtp, reFmtp)) {
                int pkt_mode = std::stoi(matchFmtp[1].str());
                std::string p_level_id = matchFmtp[2].str();
                if (caseInsensitiveStringCompare(h264_profile_level_id, p_level_id) &&
                    h264_packetization_mode == pkt_mode) {
                  keep = true;
                }
              }
            }
          } else if (caseInsensitiveStringCompare("vp9", payloadCodec)) {
            int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + vp9fmtp);
            if (fmtpLine != -1) {
              std::smatch matchFmtp;
              std::regex reFmtp(payloadNumber + vp9fmtp);
              if (std::regex_search(sdpLines[fmtpLine], matchFmtp, reFmtp)) {
                int profile_id = std::stoi(matchFmtp[1].str());
                if (vp9_profile_id == profile_id) {
                  keep = true;
                }
              }
            }
          } else {
            keep = true;
          }
        }
        std::string apt = "apt=" + payloadNumber;
        std::string rtxPayloadNumber = "";
        int aptLine = findLines(sdpLines, apt);
        if (aptLine != -1) {
          std::smatch matchApt;
          std::regex reApt("a=fmtp:([0-9]+) apt");
          if (std::regex_search(sdpLines[aptLine], matchApt, reApt)) {
            rtxPayloadNumber = matchApt[1].str();
            if (keep) {
              aptKeep = true;
            }
          }
        }
        if (keep || aptKeep) {
          if (keep) {
            payloads += " " + payloadNumber;
            video_payload_numbers.push_back(std::stoi(payloadNumber));
          }
          if (aptKeep) {
            payloads += " " + rtxPayloadNumber;
            apt_payload_numbers.push_back(std::stoi(rtxPayloadNumber));
          }
        } else if (!caseInsensitiveStringCompare(audio_codec, payloadCodec)) {
          if (std::find(video_payload_numbers.begin(), video_payload_numbers.end(), std::stoi(payloadNumber)) != video_payload_numbers.end() ||
              std::find(apt_payload_numbers.begin(), apt_payload_numbers.end(), std::stoi(payloadNumber)) != apt_payload_numbers.end()) {
            // do nothing
          } else {
            deletePayload(sdp, std::stoi(payloadNumber));
          }
        }
      }
    }
  }

  // remove all other codecs
  static void filterCodecs(
    std::string& sdp,
    std::string& payloads,
    std::vector<int>& video_payload_numbers,
    const std::initializer_list<std::string>& codecs,
    const int h264_packetization_mode,
    const std::string& h264_profile_level_id,
    const int vp9_profile_id
  )
  {
    std::string h264fmtp = " level-asymmetry-allowed=[0-1];packetization-mode=([0-9]);profile-level-id=([0-9a-f]{6})";
    std::string vp9fmtp = " profile-id=([0-9])";
    std::vector<int> apt_payload_numbers;
    std::string audio_codec = "opus";
    std::vector<std::string> sdpLines;
    split(sdp, (char*)"\r\n", sdpLines);
    int line = -1;
    for (std::string sdpLine : sdpLines) {
      ++line;
      std::smatch match;
      std::string payloadCodec;
      std::string payloadNumber;
      std::string payloadRe = "a=rtpmap:([0-9]+) ([a-z0-9-]+)";
      std::regex re(payloadRe);
      if (std::regex_search(sdpLine, match, re)) {
        payloadNumber = match[1].str();
        payloadCodec = match[2].str();
        bool found = false;
        for (auto codec : codecs) {
          if (caseInsensitiveStringCompare(codec, payloadCodec)) {
            found = true;
            break;
          }
        }
        bool keep = false;
        bool aptKeep = false;
        if (found) {
          if (caseInsensitiveStringCompare("h264", payloadCodec)) {
            int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + h264fmtp);
            if (fmtpLine != -1) {
              std::smatch matchFmtp;
              std::regex reFmtp(payloadNumber + h264fmtp);
              if (std::regex_search(sdpLines[fmtpLine], matchFmtp, reFmtp)) {
                int pkt_mode = std::stoi(matchFmtp[1].str());
                std::string p_level_id = matchFmtp[2].str();
                if (caseInsensitiveStringCompare(h264_profile_level_id, p_level_id) &&
                    h264_packetization_mode == pkt_mode) {
                  keep = true;
                }
              }
            }
          } else if (caseInsensitiveStringCompare("vp9", payloadCodec)) {
            int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + vp9fmtp);
            if (fmtpLine != -1) {
              std::smatch matchFmtp;
              std::regex reFmtp(payloadNumber + vp9fmtp);
              if (std::regex_search(sdpLines[fmtpLine], matchFmtp, reFmtp)) {
                int profile_id = std::stoi(matchFmtp[1].str());
                if (vp9_profile_id == profile_id) {
                  keep = true;
                }
              }
            }
          } else {
            keep = true;
          }
        }
        std::string apt = "apt=" + payloadNumber;
        std::string rtxPayloadNumber = "";
        int aptLine = findLines(sdpLines, apt);
        if (aptLine != -1) {
          std::smatch matchApt;
          std::regex reApt("a=fmtp:([0-9]+) apt");
          if (std::regex_search(sdpLines[aptLine], matchApt, reApt)) {
            rtxPayloadNumber = matchApt[1].str();
            if (keep) {
              aptKeep = true;
              payloads += " " + rtxPayloadNumber;
            }
          }
        }
        if (keep || aptKeep) {
          if (keep) {
            payloads += " " + payloadNumber;
            video_payload_numbers.push_back(std::stoi(payloadNumber));
          }
          if (aptKeep) {
            payloads += " " + rtxPayloadNumber;
            apt_payload_numbers.push_back(std::stoi(rtxPayloadNumber));
          }
        } else if (!caseInsensitiveStringCompare(audio_codec, payloadCodec)) {
          if (std::find(video_payload_numbers.begin(), video_payload_numbers.end(), std::stoi(payloadNumber)) != video_payload_numbers.end() ||
              std::find(apt_payload_numbers.begin(), apt_payload_numbers.end(), std::stoi(payloadNumber)) != apt_payload_numbers.end()) {
            // do nothing
          } else {
            deletePayload(sdp, std::stoi(payloadNumber));
          }
        }
      }
    }
  }

  static void processCodec(
    const std::string& desired_codec,
    const VideoCodecs videoCodec,
    std::vector<std::string>& sdpLines,
    std::string& payloads,
    std::vector<int>& video_payload_numbers
  )
  {
    return processCodec(desired_codec, videoCodec, sdpLines, payloads,
                        video_payload_numbers, 0, "42e01f", 0);
  }

  static void processCodec(
    const std::string& desired_codec,
    const VideoCodecs videoCodec,
    std::vector<std::string>& sdpLines,
    std::string& payloads,
    std::vector<int>& video_payload_numbers,
    const int vp9_profile_id
  )
  {
    return processCodec(desired_codec, videoCodec, sdpLines, payloads,
                        video_payload_numbers, 0, "42e01f", vp9_profile_id);
  }

  static void processCodec(
    const std::string& desired_codec,
    const VideoCodecs videoCodec,
    std::vector<std::string>& sdpLines,
    std::string& payloads,
    std::vector<int>& video_payload_numbers,
    const int h264_packetization_mode,
    const std::string& h264_profile_level_id
  )
  {
    return processCodec(desired_codec, videoCodec, sdpLines, payloads,
                        video_payload_numbers, h264_packetization_mode,
                        h264_profile_level_id, 0);
  }

  static void processCodec(
    const std::string& desired_codec,
    const VideoCodecs videoCodec,
    std::vector<std::string>& sdpLines,
    std::string& payloads,
    std::vector<int>& video_payload_numbers,
    const int h264_packetization_mode,
    const std::string& h264_profile_level_id,
    const int vp9_profile_id
  )
  {
    int found;
    bool keep = false;
    std::smatch match, matchApt;
    std::string codec, payload, payloadNumber, rtxPayloadNumber;

    const std::string h264_id = h264_profile_level_id;
    const int h264_pm = h264_packetization_mode;
    const int vp9_id = vp9_profile_id;

    std::string h264fmtp = " level-asymmetry-allowed=[0-1];packetization-mode=([0-9]);profile-level-id=([0-9a-f]{6})";
    std::string vp9fmtp = " profile-id=([0-9])";

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
          keep = false;
          if (caseInsensitiveStringCompare(codec, "h264")) {
            int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + h264fmtp);
            if (fmtpLine != -1) {
              std::regex reFmtp(payloadNumber + h264fmtp);
              if (std::regex_search(sdpLines[fmtpLine], match, reFmtp)) {
                int pkt_mode = std::stoi(match[1].str());
                std::string p_level_id = match[2].str();
                if (caseInsensitiveStringCompare(desired_codec, codec) &&
                    caseInsensitiveStringCompare(h264_id, p_level_id) &&
                    h264_pm == pkt_mode) {
                  keep = true;
                }
              }
            }
          } else if (codec == "vp9") {
            int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + vp9fmtp);
            if (fmtpLine != -1) {
              std::regex reFmtp(payloadNumber + vp9fmtp);
              if (std::regex_search(sdpLines[fmtpLine], match, reFmtp)) {
                int profile_id = std::stoi(match[1].str());
                if (caseInsensitiveStringCompare(desired_codec, codec) &&
                    vp9_id == profile_id) {
                  keep = true;
                }
              }
            }
          } else {
            if (caseInsensitiveStringCompare(desired_codec, codec))
              keep = true;
          }
          if (keep) {
            payloads += " " + payloadNumber;
            video_payload_numbers.push_back(std::stoi(payloadNumber));
          } else {
            deletePayload(sdpLines, std::stoi(payloadNumber));
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

  static void deleteCodec(
    std::string& sdp,
    const std::string& codec,
    std::string& payloads,
    std::vector<int>& video_payload_numbers
  )
  {
    int line;
    std::vector<std::string> sdpLines;
    split(sdp, (char*)"\r\n", sdpLines);
    do {
      line = findLines(sdpLines, "rtpmap");
      bool keep = false;
      std::smatch match;
      std::string payloadCodec;
      std::string payloadNumber;
      const char* payloadRe = "a=rtpmap:([0-9]+) ([a-z0-9-]+)";
      auto re = std::regex{payloadRe, std::regex_constants::icase};
      if (std::regex_search(sdpLines[line], match, re)) {
        payloadNumber = match[1].str();
        payloadCodec = match[2].str();
      }
      if (caseInsensitiveStringCompare(codec, payloadCodec)) {
        deletePayload(sdpLines, std::stoi(payloadNumber));
      } else {
        keep = true;
        payloads += " " + payloadNumber;
        video_payload_numbers.push_back(std::stoi(payloadNumber));
      }
      std::string apt = "apt=" + payloadNumber;
      int aptLine = findLines(sdpLines, apt);
      if (aptLine != -1) {
        std::smatch matchApt;
        std::regex reApt("a=fmtp:([0-9]+) apt");
        if (std::regex_search(sdpLines[aptLine], matchApt, reApt)) {
          std::string rtxPayloadNumber = matchApt[1].str();
          if (keep) {
            payloads += " " + rtxPayloadNumber;
          } else {
            sdpLines.erase(sdpLines.begin() + aptLine);
            sdpLines.erase(sdpLines.begin() + aptLine - 1);
          }
        }
      }
    } while (line != -1);
    sdp = join(sdpLines, "\r\n");
  }

  static bool caseInsensitiveStringCompare(const char* str1, const char* str2)
  {
    return caseInsensitiveStringCompare(std::string(str1), std::string(str2));
  }

  static bool caseInsensitiveStringCompare(const std::string& s1, const std::string& s2)
  {
    std::string s1Cpy(s1);
    std::string s2Cpy(s2);
    std::transform(s1Cpy.begin(), s1Cpy.end(), s1Cpy.begin(), ::tolower);
    std::transform(s2Cpy.begin(), s2Cpy.end(), s2Cpy.begin(), ::tolower);
    return (s1Cpy == s2Cpy);
  }

  static int findLines(const std::vector<std::string>& sdpLines, std::string prefix)
  {
    for (unsigned long i = 0; i < sdpLines.size(); i++) {
      if (sdpLines[i].find(prefix) != std::string::npos)
        return i;
    }
    return -1;
  }

  static int findLinesRegEx(const std::vector<std::string>& sdpLines, std::string prefix)
  {
    std::regex re(prefix);
    for (unsigned long i = 0; i < sdpLines.size(); i++) {
      std::smatch match;
      if (std::regex_search(sdpLines[i], match, re))
        return i;
    }
    return -1;
  }

  static std::string join(std::vector<std::string>& v, std::string delim)
  {
    std::ostringstream s;
    for (const auto& i : v) {
      if (&i != &v[0])
        s << delim;
      s << i;
    }
    s << delim;
    return s.str();
  }

  static void split(const std::string& s, char* delim, std::vector<std::string>& v)
  {
    char* dup = strdup(s.c_str());
    char* token = strtok(dup, delim);
    while(token != NULL) {
      v.push_back(std::string(token));
      token = strtok(NULL, delim);
    }
    free(dup);
  }
};

#endif
