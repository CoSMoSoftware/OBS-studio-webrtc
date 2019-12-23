#ifndef _WEBRTCSTREAM_H_
#define _WEBRTCSTREAM_H_

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

#include "api/create_peerconnection_factory.h"
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/set_remote_description_observer_interface.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/platform_file.h"
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
  public webrtc::SetSessionDescriptionObserver,
  public webrtc::SetRemoteDescriptionObserverInterface {};

class WebRTCStream : public rtc::RefCountedObject<WebRTCStreamInterface> {
public:
  enum Type {
    Janus     = 0,
    Wowza     = 1,
    Millicast = 2,
    Evercast  = 3
  };

  WebRTCStream(obs_output_t *output);
  ~WebRTCStream() override;

  bool close(bool wait);
  bool start(Type type);
  bool stop();
  void onAudioFrame(audio_data *frame);
  void onVideoFrame(video_data *frame);
  void setCodec(const std::string &new_codec) { this->video_codec = new_codec; }

  //
  // WebsocketClient::Listener implementation.
  //
  void onConnected() override;
  void onDisconnected() override;
  void onLogged(int code) override;
  void onLoggedError(int code) override;
  void onOpened(const std::string &sdp) override;
  void onOpenedError(int code) override;
  void onRemoteIceCandidate(const std::string &sdpData) override;

  //
  // PeerConnectionObserver implementation.
  //
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState /* new_state */) override {}
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream */) override {}
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream */) override {}
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> /* channel */) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState /* new_state */) override {}
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState /* new_state */) override {}
  void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;
  void OnIceConnectionReceivingChange(bool /* receiving */) override {}

  // CreateSessionDescriptionObserver
  void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;

  // CreateSessionDescriptionObserver / SetSessionDescriptionObserver
  void OnFailure(const std::string &error) override;

  // SetSessionDescriptionObserver
  void OnSuccess() override;

  // SetRemoteDescriptionObserverInterface
  void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

  // NOTE LUDO: #80 add getStats
  // WebRTC stats
  void getStats();

  // Bitrate & dropped frames
  uint64_t getBitrate();
  int getDroppedFrames();

  // RTCDataChannelStats
  uint32_t get_data_messages_sent()     { return data_messages_sent; }
  uint64_t get_data_bytes_sent()        { return data_bytes_sent; }
  uint32_t get_data_messages_received() { return data_messages_received; }
  uint64_t get_data_bytes_received()    { return data_bytes_received; }

  // RTCMediaStreamTrackStats
  // double   get_track_jitter_buffer_delay()           { return track_jitter_buffer_delay; }
  // uint64_t get_track_jitter_buffer_emitted_count()   { return track_jitter_buffer_emitted_count; }
  // Video-only members
  uint32_t get_track_frame_width()                   { return track_frame_width; }
  uint32_t get_track_frame_height()                  { return track_frame_height; }
  // double   get_track_frames_per_second()             { return track_frames_per_second; }
  uint32_t get_track_frames_sent()                   { return track_frames_sent; }
  uint32_t get_track_huge_frames_sent()              { return track_huge_frames_sent; }
  // uint32_t get_track_frames_received()               { return track_frames_received; }
  // uint32_t get_track_frames_decoded()                { return track_frames_decoded; }
  // uint32_t get_track_frames_dropped()                { return track_frames_dropped; }
  // uint32_t get_track_frames_corrupted()              { return track_frames_corrupted; }
  // uint32_t get_track_partial_frames_lost()           { return track_partial_frames_lost; }
  // uint32_t get_track_full_frames_lost()              { return track_full_frames_lost; }
  // Audio-only members
  double   get_track_audio_level()                   { return track_audio_level; }
  double   get_track_total_audio_energy()            { return track_total_audio_energy; }
  // double   get_track_echo_return_loss()              { return track_echo_return_loss; }
  // double   get_track_echo_return_loss_enhancement()  { return track_echo_return_loss_enhancement; }
  // uint64_t get_track_total_samples_received()        { return track_total_samples_received; }
  double   get_track_total_samples_duration()        { return track_total_samples_duration; }
  // uint64_t get_track_concealed_samples()             { return track_concealed_samples; }
  // uint64_t get_track_concealment_events()            { return track_concealment_events; }
  // Non-standard audio-only member

  // RTCPeerConnectionStats
  uint32_t get_data_channels_opened() { return data_channels_opened; }
  uint32_t get_data_channels_closed() { return data_channels_closed; }

  // Synchronously get stats
  rtc::scoped_refptr<const webrtc::RTCStatsReport> NewGetStats();

  template <typename T>
  rtc::scoped_refptr<T> make_scoped_refptr(T *t) {
    return rtc::scoped_refptr<T>(t);
  }

private:
  // Connection properties
  Type type;
  int audio_bitrate;
  int video_bitrate;
  std::string url;
  std::string room;
  std::string username;
  std::string password;
  std::string protocol;
  std::string audio_codec;
  std::string video_codec;

  // NOTE LUDO: #80 add getStats
  // RTCOutboundRTPStreamStat
  uint16_t frame_id;
  uint64_t audio_bytes_sent;
  uint64_t video_bytes_sent;
  uint64_t total_bytes_sent;
  int      pli_received; // Picture Loss Indication
  uint32_t packets_sent;
  // double   target_bitrate;
  // uint32_t frames_encoded;

  // RTCDataChannelStats
  uint32_t data_messages_sent;
  uint64_t data_bytes_sent;
  uint32_t data_messages_received;
  uint64_t data_bytes_received;

  // RTCMediaStreamTrackStats
  // double   track_jitter_buffer_delay;
  // uint64_t track_jitter_buffer_emitted_count;
  // Video-only members
  uint32_t track_frame_width;
  uint32_t track_frame_height;
  // double   track_frames_per_second;
  uint32_t track_frames_sent;
  uint32_t track_huge_frames_sent;
  uint32_t track_frames_received;
  uint32_t track_frames_decoded;
  uint32_t track_frames_dropped;
  uint32_t track_frames_corrupted;
  uint32_t track_partial_frames_lost;
  uint32_t track_full_frames_lost;
  // Audio-only members
  double   track_audio_level;
  double   track_total_audio_energy;
  double   track_echo_return_loss;
  double   track_echo_return_loss_enhancement;
  uint64_t track_total_samples_received;
  double   track_total_samples_duration;
  uint64_t track_concealed_samples;
  uint64_t track_concealment_events;
  // Non-standard audio-only member
  uint64_t track_jitter_buffer_flushes;
  uint64_t track_delayed_packet_outage_samples;

  // RTCPeerConnectionStats
  uint32_t data_channels_opened;
  uint32_t data_channels_closed;

  rtc::CriticalSection crit_;

  // Audio Wrapper
  rtc::scoped_refptr<AudioDeviceModuleWrapper> adm;

  // Video Capturer
  rtc::scoped_refptr<VideoCapturer> videoCapturer;
  rtc::TimestampAligner timestamp_aligner_;

  // PeerConnection
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;

  // SetRemoteDescription observer
  rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface> srd_observer;

  // Media stream
  rtc::scoped_refptr<webrtc::MediaStreamInterface> stream;

  // Tracks
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track;

  // WebRTC threads
  std::unique_ptr<rtc::Thread> network;
  std::unique_ptr<rtc::Thread> worker;
  std::unique_ptr<rtc::Thread> signaling;

  // Websocket client
  WebsocketClient *client;

  // OBS stream output
  obs_output_t *output;
};

#endif
