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

    // Bitrate & dropped frames
    uint64_t getBitrate();
    int getDroppedFrames();

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

    uint16_t frame_id;
    uint64_t audio_bytes_sent;
    uint64_t video_bytes_sent;
    uint64_t total_bytes_sent;
    int pli_received; // Picture Loss Indication

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
