#ifndef _WEBRTCSTREAM_H_
#define _WEBRTCSTREAM_H_

#if WIN32
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "Msdmo.lib")
#pragma comment(lib, "dmoguids.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "amstrmid.lib")
#endif

// lib obs includes
#include "obs.h"

#if WIN32
// undef M_PI o avoid Warning C4005 'M_PI': macro redefinition
#if defined(M_PI)
#undef M_PI
#endif
#endif

// obs-webrtc includes
#include "websocket-client/WebsocketClient.h"
#include "VideoCapturer.h"
#include "AudioDeviceModuleWrapper.h"
#include "obsWebrtcAudioSource.h"

// webrtc includes
#include "api/create_peerconnection_factory.h"
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "api/rtc_error.h"
#include "api/scoped_refptr.h"
#include "api/set_remote_description_observer_interface.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/synchronization/mutex.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "rtc_base/timestamp_aligner.h"

// std lib
#include <initializer_list>
#include <regex>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

class WebRTCStreamInterface
	: public WebsocketClient::Listener,
	  public webrtc::PeerConnectionObserver,
	  public webrtc::CreateSessionDescriptionObserver,
	  public webrtc::SetSessionDescriptionObserver,
	  public webrtc::SetRemoteDescriptionObserverInterface {};

class WebRTCStream : public rtc::RefCountedObject<WebRTCStreamInterface> {
public:
	enum Type { Millicast = 0, CustomWebrtc = 1 };

	WebRTCStream(obs_output_t *output);
	~WebRTCStream() override;

	bool close(bool wait);
	bool start(Type type);
	bool stop();
	void onAudioFrame(audio_data *frame);
	void onVideoFrame(video_data *frame);
	void setCodec(const std::string &new_codec)
	{
		this->video_codec = new_codec;
	}

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
	void OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState /* new_state */)
		override
	{
	}
	void OnAddStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream */)
		override
	{
	}
	void OnRemoveStream(
		rtc::scoped_refptr<webrtc::MediaStreamInterface> /* stream */)
		override
	{
	}
	void OnDataChannel(
		rtc::scoped_refptr<webrtc::DataChannelInterface> /* channel */)
		override
	{
	}
	void OnRenegotiationNeeded() override {}
	void OnIceConnectionChange(
		webrtc::PeerConnectionInterface::
			IceConnectionState /* new_state */) override;
	void
	OnIceGatheringChange(webrtc::PeerConnectionInterface::
				     IceGatheringState /* new_state */) override
	{
	}
	void
	OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;
	void OnIceConnectionReceivingChange(bool /* receiving */) override {}
	void OnConnectionChange(
		webrtc::PeerConnectionInterface::PeerConnectionState new_state)
		override;

	// CreateSessionDescriptionObserver
	void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;

	// CreateSessionDescriptionObserver / SetSessionDescriptionObserver
	void OnFailure(webrtc::RTCError error) override;

	// SetSessionDescriptionObserver
	void OnSuccess() override;

	// SetRemoteDescriptionObserverInterface
	void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

	// NOTE LUDO: #80 add getStats
	// WebRTC stats
	void getStats();
	const char *get_stats_list() { return stats_list_.c_str(); }
	// Bitrate & dropped frames
	uint64_t getBitrate() { return total_bytes_sent_; }
	uint32_t getDroppedFrames() { return pli_received_; }
	// #310 webrtc getstats()
	uint64_t getTransportBytesSent() { return transport_bytes_sent_; }
	uint64_t getTransportBytesReceived()
	{
		return transport_bytes_received_;
	}
	uint64_t getVideoPacketsSent() { return video_packets_sent_; }
	uint64_t getVideoBytesSent() { return video_bytes_sent_; }
	uint64_t getVideoFirCount() { return video_fir_count_; }
	uint32_t getVideoPliCount() { return pli_received_; }
	uint64_t getVideoNackCount() { return video_nack_count_; }
	uint64_t getVideoQpSum() { return video_qp_sum_; }
	uint64_t getAudioBytesSent() { return audio_bytes_sent_; }
	uint64_t getAudioPacketsSent() { return audio_packets_sent_; }
	uint32_t getTrackAudioLevel() { return track_audio_level_; }
	uint32_t getTrackTotalAudioEnergy()
	{
		return track_total_audio_energy_;
	}
	uint32_t getTrackTotalSamplesDuration()
	{
		return track_total_samples_duration_;
	}
	uint32_t getTrackFrameWidth() { return track_frame_width_; }
	uint32_t getTrackFrameHeight() { return track_frame_height_; }
	uint64_t getTrackFramesSent() { return track_frames_sent_; }
	uint64_t getTrackHugeFramesSent() { return track_huge_frames_sent_; }

	// Synchronously get stats
	std::vector<rtc::scoped_refptr<const webrtc::RTCStatsReport>>
	NewGetStats();

	template<typename T> rtc::scoped_refptr<T> make_scoped_refptr(T *t)
	{
		return rtc::scoped_refptr<T>(t);
	}

	// OBS stream output
	obs_output_t *output;

private:
	// Count number of video sources in current scene
	int getVideoSourceCount() const;

	void deliver_video_frame(video_data *frame);

	// Audio/video synchronisation management
	bool audio_started_;
	uint64_t last_delivered_audio_ts_;
	std::queue<video_data *> video_queue_;
	void enqueue_frame(video_data *frame);
	void process_video_queue();
	// video_queue_ is shared by audio thread and video thread
	// ==> Protect access to video_queue_ with a mutex
	std::mutex mutex_video_queue_;
	// Method deliver_video_frame() can be called by video thread
	// and also by audio thread for video frames that have been buffered
	// to synchronize with audio timestamps.
	// Critical section: make sure only one thread at a time call method deliver_video_frame()
	// by protecting it with a lock on mutex_deliver_video_frame_.
	std::mutex mutex_deliver_video_frame_;

	// Connection properties
	Type type;
	int audio_bitrate_;
	int video_bitrate_;
	std::string url;
	std::string room;
	std::string username;
	std::string password;
	std::string protocol;
	std::string audio_codec;
	std::string video_codec;
	bool simulcast_;
	bool activate_bwe_;
	int total_bitrate_;
	const int MULTIPLIER = 1000;
	bool multisource_;
	std::string sourceId_;
	std::string publishApiUrl;
	int channel_count;
	std::string colorFormat;
	// Codec profile to support the selected color format:
	// VP9 profile 0 for color format NV12 ot I420
	// VP9 profile 3 for color format I444
	int profile;

	void resetStats();

	const uint32_t audio_samplerate_ = 48000;

	// NOTE LUDO: #80 add getStats
	std::string stats_list_;
	uint16_t frame_id_;
	uint64_t total_bytes_sent_;
	uint32_t pli_received_;
	// #310 webrtc getstats()
	uint64_t transport_bytes_sent_;
	uint64_t transport_bytes_received_;
	uint64_t video_packets_sent_;
	uint64_t video_bytes_sent_;
	uint64_t video_fir_count_;
	uint64_t video_nack_count_;
	uint64_t video_qp_sum_;
	uint64_t audio_packets_sent_;
	uint64_t audio_bytes_sent_;
	uint32_t track_audio_level_;
	uint32_t track_total_audio_energy_;
	uint32_t track_total_samples_duration_;
	uint32_t track_frame_width_;
	uint32_t track_frame_height_;
	uint64_t track_frames_sent_;
	uint64_t track_huge_frames_sent_;
	// Used to compute fps
	std::chrono::system_clock::time_point previous_time_;
	uint32_t previous_frames_sent_ = 0;

	std::thread thread_closeAsync;

	webrtc::Mutex crit_;

	// Audio Wrapper
	rtc::scoped_refptr<AudioDeviceModuleWrapper> adm;

	// Video Capturer
	rtc::scoped_refptr<VideoCapturer> videoCapturer;
	rtc::TimestampAligner timestamp_aligner_;

	// PeerConnection
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc;

	// SetRemoteDescription observer
	rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface>
		srd_observer;

	// Media stream
	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream;

	// Webrtc Source that wraps an OBS capturer
	rtc::scoped_refptr<obsWebrtcAudioSource> audio_source;

	// Tracks
	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track;
	rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track;

	// WebRTC threads
	std::unique_ptr<rtc::Thread> network;
	std::unique_ptr<rtc::Thread> worker;
	std::unique_ptr<rtc::Thread> signaling;

	// Websocket client
	WebsocketClient *client;
};

#endif
