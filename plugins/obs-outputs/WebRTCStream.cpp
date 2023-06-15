// Copyright Dr. Alex. Gouaillard (2015, 2020)

#include "WebRTCStream.h"
#include "SDPModif.h"
#include "useless_network_factory.h"
#include "webrtc_pc_factory_di_helpers.h"

#include "media-io/video-io.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video/i420_buffer.h"
#include "api/video/i444_buffer.h"
#include "api/video/i010_buffer.h"
#include "pc/rtc_stats_collector.h"
#include "rtc_base/checks.h"
#include "system_wrappers/include/field_trial.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include <libyuv.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <memory>
#include <mutex>
#include <thread>
#include <algorithm>
#include <locale>

#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR, format, ##__VA_ARGS__)

class StatsCallback : public webrtc::RTCStatsCollectorCallback {
public:
	rtc::scoped_refptr<const webrtc::RTCStatsReport> report()
	{
		return report_;
	}
	bool called() const { return called_; }

protected:
	void OnStatsDelivered(
		const rtc::scoped_refptr<const webrtc::RTCStatsReport> &report)
		override
	{
		report_ = report;
		called_ = true;
	}

private:
	bool called_ = false;
	rtc::scoped_refptr<const webrtc::RTCStatsReport> report_;
};

class CustomLogger : public rtc::LogSink {
public:
	void OnLogMessage(const std::string &message) override
	{
		info("%s", message.c_str());
	}
};

CustomLogger logger;

WebRTCStream::WebRTCStream(obs_output_t *output)
{
	rtc::LogMessage::RemoveLogToStream(&logger);
	rtc::LogMessage::AddLogToStream(&logger,
					rtc::LoggingSeverity::LS_VERBOSE);

	resetStats();

	profile_ = 0;
	colorFormat_ = "NV12";
	audio_bitrate_ = 128; // kbps
	video_bitrate_ = 2500; // kbps

	activate_bwe_ = false;
	total_bitrate_ = (audio_bitrate_ + video_bitrate_) * MULTIPLIER; // bps

	// Store output
	this->output = output;
	this->client = nullptr;

	// Create audio device module
	// NOTE ALEX: check if we still need this
	adm = new rtc::RefCountedObject<AudioDeviceModuleWrapper>();

	// Network thread
	network = rtc::Thread::CreateWithSocketServer();
	network->SetName("network", nullptr);
	network->Start();

	// Worker thread
	worker = rtc::Thread::Create();
	worker->SetName("worker", nullptr);
	worker->Start();

	// Signaling thread
	signaling = rtc::Thread::Create();
	signaling->SetName("signaling", nullptr);
	signaling->Start();

	auto deps = webrtc::CreateDefaultPeerConnectionFactoryDependencies(
		network.get(), worker.get(), signaling.get(), adm,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		webrtc::CreateBuiltinAudioDecoderFactory(),
		webrtc::CreateBuiltinVideoEncoderFactory(),
		webrtc::CreateBuiltinVideoDecoderFactory(), nullptr, nullptr);

	deps.network_controller_factory = std::make_unique<webrtc::UselessNetworkControllerFactory>();
	factory = webrtc::CreateModularPeerConnectionFactory(std::move(deps));

	// Create video capture module
	videoCapturer = new rtc::RefCountedObject<VideoCapturer>();

	// First video frame not yet received
	first_frame_received_ = false;

	// Initialize audio/video synchronisation
	audio_started_ = false;
	last_delivered_audio_ts_ = 0;
}

WebRTCStream::~WebRTCStream()
{
	rtc::LogMessage::RemoveLogToStream(&logger);

	// Shutdown websocket connection and close Peer Connection
	close(false);

	// Free factories
	adm = nullptr;
	pc = nullptr;
	factory = nullptr;
	videoCapturer = nullptr;

	// Stop all threads
	if (!network->IsCurrent())
		network->Stop();
	if (!worker->IsCurrent())
		worker->Stop();
	if (!signaling->IsCurrent())
		signaling->Stop();

	network.release();
	worker.release();
	signaling.release();
}

void WebRTCStream::resetStats()
{
	stats_list_ = "";
	frame_id_ = 0;
	pli_received_ = 0;
	total_bytes_sent_ = 0;
	// #310 webrtc getstats()
	transport_bytes_sent_ = 0;
	transport_bytes_received_ = 0;
	video_packets_sent_ = 0;
	video_bytes_sent_ = 0;
	video_fir_count_ = 0;
	video_nack_count_ = 0;
	video_qp_sum_ = 0;
	audio_packets_sent_ = 0;
	audio_bytes_sent_ = 0;
	track_audio_level_ = 0;
	track_total_audio_energy_ = 0;
	track_total_samples_duration_ = 0;
	track_frame_width_ = 0;
	track_frame_height_ = 0;
	track_frames_sent_ = 0;
	track_huge_frames_sent_ = 0;
	previous_time_ = std::chrono::system_clock::time_point(
		std::chrono::duration<int>(0));
	previous_frames_sent_ = 0;
}

bool WebRTCStream::start(WebRTCStream::Type type)
{
	info("WebRTCStream::start");
	this->type = type;

	resetStats();

	// First video frame not yet received
	first_frame_received_ = false;

	// Initialize audio/video synchronisation
	audio_started_ = false;
	last_delivered_audio_ts_ = 0;

	// Access service if started, or fail

	obs_service_t *service = obs_output_get_service(output);
	if (!service) {
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				output,
				"An unexpected error occurred during stream startup.");
			obs_output_signal_stop(output,
					       OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	}

	// Extract setting from service

	url = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_SERVER_URL)
		? obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_SERVER_URL)
		: "";
	room = obs_service_get_room(service) ? obs_service_get_room(service)
					     : "";
	username = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_USERNAME)
		? obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_USERNAME)
		: "";
	password = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_PASSWORD)
		? obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_PASSWORD)
		: "";
	video_codec = obs_service_get_codec(service)
			      ? obs_service_get_codec(service)
			      : "";
	// #271 do not list video codec H264 if it is not available in libwebrtc
#ifdef DISABLE_WEBRTC_H264
	if ("h264" == video_codec) {
		info("No video codec selected and H264 not available: set to VP9 by default");
		video_codec = "VP9";
	}
#endif
	protocol = obs_service_get_protocol(service)
			   ? obs_service_get_protocol(service)
			   : "";
	simulcast_ = obs_service_get_simulcast(service)
			     ? obs_service_get_simulcast(service)
			     : false;
	// Activate Google congestion control BWE? true ==> yes / false ==> no
	activate_bwe_ = obs_service_get_bwe(service)
							? obs_service_get_bwe(service)
							: false;
	total_bitrate_ = 0;
	multisource_ = obs_service_get_multisource(service)
			       ? obs_service_get_multisource(service)
			       : false;
	sourceId_ = obs_service_get_sourceId(service)
			    ? obs_service_get_sourceId(service)
			    : "";
	publishApiUrl = obs_service_get_publishApiUrl(service)
				? obs_service_get_publishApiUrl(service)
				: "";
	if (type == WebRTCStream::Type::CustomWebrtc && publishApiUrl.empty()) {
		publishApiUrl = url;
	}

	config_t *obs_config = obs_frontend_get_profile_config();
	if (obs_config) {
		const char *tmp =
			config_get_string(obs_config, "Video", "ColorFormat");
		if (tmp) {
			// Possible values for ColorFormat:
			// I420
			// I444
			// I010
			// NV12
			// RGB
			switch (tmp[0]) {
			case 'I': // I420, I444, I010
				if ('4' == tmp[1]) {
					if ('2' == tmp[2]) {
						colorFormat_ = "I420";
						profile_ = 0;
					} else {
						colorFormat_ = "I444";
						profile_ = 1;
					}
				} else {
					colorFormat_ = "I010";
					profile_ = 2;
				}
				break;
			case 'N': // NV12
				colorFormat_ = "NV12";
				profile_ = 0;
				break;
			case 'R': // RGB
				colorFormat_ = "RGB";
				profile_ = 0;
				break;
			case 'P': // P010: color format not supported
				colorFormat_ = "P010";
				break;
			default:
				colorFormat_ = "NV12";
				profile_ = 0;
			}
		} else {
			// No color format read from config file: Use NV12 color format by default
			colorFormat_ = "NV12";
			profile_ = 0;
		}
	} else {
		// No config file: Use NV12 color format by default
		colorFormat_ = "NV12";
		profile_ = 0;
	}

	if ("P010" == colorFormat_) {
		error("Error color format P010 not supported");
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				output,
				"Error: color format P010 is not supported, please select a different color format.");
			obs_output_signal_stop(output,
					       OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	}

	if (("I444" == colorFormat_ || "I010" == colorFormat_)
			&& "VP9" != video_codec) {
		info("Color format %s is not supported for video codec %s: Switching to color format I420\n",
		     colorFormat_.c_str(), video_codec.c_str());
	}

	// No Simulast for VP9 (not supported properly by libwebrtc) and AV1 codecs
	if (simulcast_ && (video_codec.empty() || "VP9" == video_codec ||
			   "AV1" == video_codec)) {
		info("Simulcast not supported for %s: Disabling Simulcast\n",
		     video_codec.empty() ? "VP9" : video_codec.c_str());
		simulcast_ = false;
	}

	// Some extra log

	info("Color format: %s", colorFormat_.c_str());
	info("Video codec: %s",
	     video_codec.empty() ? "Automatic" : video_codec.c_str());
	if (0 == getVideoSourceCount()) {
		info("No video source in current scene");
	}
	info("Simulcast: %s", simulcast_ ? "true" : "false");
	info("Congestion control bandwidth estimation activated: %s", activate_bwe_ ? "true" : "false");
	if (!activate_bwe_) {
		info("Total bitrate (audio+video):  %d bps", total_bitrate_);
	}
	if (multisource_) {
		info("Multisource: true, sourceID: %s", sourceId_.c_str());
	} else {
		info("Multisource: false");
	}
	info("Publish API URL: %s", publishApiUrl.c_str());
	info("Protocol:    %s",
	     protocol.empty() ? "Automatic" : protocol.c_str());

	// Stream setting sanity check

	bool isServiceValid = true;
	if (type != WebRTCStream::Type::Millicast) {
		if (publishApiUrl.empty()) {
			warn("Invalid publish API URL");
			isServiceValid = false;
		}
		if (type != WebRTCStream::Type::CustomWebrtc) {
			if (room.empty()) {
				warn("Missing room ID");
				isServiceValid = false;
			}
		}
		if (type != WebRTCStream::Type::CustomWebrtc) {
			if (password.empty()) {
				warn("Missing Password");
				isServiceValid = false;
			}
		}
	}

	if (!isServiceValid) {
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				output,
				"Your service settings are not complete. Open the settings => stream window and complete them.");
			obs_output_signal_stop(output,
					       OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	}

	// Set up encoders.
	// NOTE ALEX: should not be done for webrtc.

	obs_output_t *context = output;

	obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
	obs_data_t *asettings = obs_encoder_get_settings(aencoder);
	audio_bitrate_ = (int)obs_data_get_int(asettings, "bitrate"); // kbps
	obs_data_release(asettings);

	obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
	obs_data_t *vsettings = obs_encoder_get_settings(vencoder);
	video_bitrate_ = (int)obs_data_get_int(vsettings, "bitrate"); // kbps
	obs_data_release(vsettings);

	total_bitrate_ = (audio_bitrate_ + video_bitrate_) * MULTIPLIER; // bps

	struct obs_audio_info audio_info;
	if (!obs_get_audio_info(&audio_info)) {
		warn("Failed to load audio settings.  Defaulting to opus.");
		audio_codec = "opus";
	} else {
		// NOTE ALEX: if input # channel > output we should down mix
		//            if input is > 2 but < 6 we might have a porblem with multiopus.
		channel_count = (int)(audio_info.speakers);
		audio_codec = channel_count <= 2 ? "opus" : "multiopus";
	}

	// Shutdown websocket connection and close Peer Connection (just in case)
	if (close(false)) {
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
		});
		thread.detach();
	}

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	webrtc::PeerConnectionInterface::IceServer server;
	server.urls = {"stun:stun.l.google.com:19302"};
	config.servers.push_back(server);
	// config.bundle_policy = webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle;
	// config.disable_ipv6 = true;
	// config.rtcp_mux_policy = webrtc::PeerConnectionInterface::kRtcpMuxPolicyRequire;
	config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	// config.set_cpu_adaptation(false);
	// config.set_suspend_below_min_bitrate(false);

	webrtc::PeerConnectionDependencies dependencies(this);

	// Call InitFieldTrialsFromString before creating peer connection
	if (activate_bwe_) {
		// Activate Google BWE
		constexpr const char *network_field_trials = "WebRTC-Video-Pacing/max_delay:2000/";
		webrtc::field_trial::InitFieldTrialsFromString(network_field_trials);
	} else {
		// Deactivate Google BWE
		constexpr const char *network_field_trials = "WebRTC-Bwe-InjectedCongestionController/Enabled/"
																								"WebRTC-Video-Pacing/max_delay:0/";
		webrtc::field_trial::InitFieldTrialsFromString(network_field_trials);
	}

	pc = factory->CreatePeerConnection(config, std::move(dependencies));

	if (!pc.get()) {
		error("Error creating Peer Connection");
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				output,
				"There was an error connecting to the server. Are you connected to the internet?");
			obs_output_signal_stop(output,
					       OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	} else {
		info("PEER CONNECTION CREATED\n");
	}

	if (!activate_bwe_) {
		// No congestion control BWE ==> set bitrates as defined by the user
		webrtc::BitrateSettings bitrates;
		bitrates.min_bitrate_bps = total_bitrate_;
		bitrates.max_bitrate_bps = total_bitrate_;
		bitrates.start_bitrate_bps = total_bitrate_;
		pc->SetBitrate(bitrates);
	}

	cricket::AudioOptions options;
	options.echo_cancellation.emplace(false); // default: true
	options.auto_gain_control.emplace(false); // default: true
	options.noise_suppression.emplace(false); // default: true
	options.highpass_filter.emplace(false);   // default: true
	options.stereo_swapping.emplace(false);
	// m104 options.typing_detection.emplace(false); // default: true
	// m100 options.experimental_agc.emplace(false);
	// m79 options.extended_filter_aec.emplace(false);
	// m79 options.delay_agnostic_aec.emplace(false);
	// m100 options.experimental_ns.emplace(false);
	// m104 options.residual_echo_detector.emplace(false); // default: true
	// options.tx_agc_limiter.emplace(false);

	stream = factory->CreateLocalMediaStream("obs");

	audio_source = obsWebrtcAudioSource::Create(&options);
	audio_track = factory->CreateAudioTrack("audio", audio_source.get());
	// pc->AddTrack(audio_track, {"obs"});
	stream->AddTrack(audio_track);

	if (getVideoSourceCount() > 0) {
		video_track = factory->CreateVideoTrack("video", videoCapturer.get());
		// pc->AddTrack(video_track, {"obs"});
		stream->AddTrack(video_track);
	}

	//Add audio track
	webrtc::RtpTransceiverInit audio_init;
	audio_init.stream_ids.push_back(stream->id());
	audio_init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
	pc->AddTransceiver(audio_track, audio_init);

	if (getVideoSourceCount() > 0) {
		//Add video track
		webrtc::RtpTransceiverInit video_init;
		video_init.stream_ids.push_back(stream->id());
		video_init.direction =
			webrtc::RtpTransceiverDirection::kSendOnly;
		if (simulcast_) {
			webrtc::RtpEncodingParameters large;
			webrtc::RtpEncodingParameters medium;
			webrtc::RtpEncodingParameters small;
			large.rid = "L";
			large.scale_resolution_down_by = 1;
			large.max_bitrate_bps = video_bitrate_ * 1000;
			medium.rid = "M";
			medium.scale_resolution_down_by = 2;
			medium.max_bitrate_bps = video_bitrate_ * 1000 / 3;
			small.rid = "S";
			small.scale_resolution_down_by = 4;
			small.max_bitrate_bps = video_bitrate_ * 1000 / 9;
			//In reverse order so large is dropped first on low network condition
			// Send small resolution only if output video resolution >= 480p
			if (obs_output_get_height(output) >= 480) {
				video_init.send_encodings.push_back(small);
			}
			video_init.send_encodings.push_back(medium);
			video_init.send_encodings.push_back(large);
		}
		pc->AddTransceiver(video_track, video_init);
	}

	client = createWebsocketClient(type);
	if (!client) {
		warn("Error creating Websocket client");
		// Close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				output,
				"There was a problem creating the websocket connection.  Are you behind a firewall?");
			obs_output_signal_stop(output,
					       OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	}

	// Extra logging

	if (type == WebRTCStream::Type::Millicast) {
		// #323: Do not log publishing token
		// info("Stream Name:      %s\nPublishing Token: %s\n",
		//      username.c_str(), password.c_str());
		info("Stream Name: %s\n", username.c_str());
		url = publishApiUrl;
	}
	info("CONNECTING TO %s", url.c_str());

	// Connect to the signalling server
	if (!client->connect(url, room, username, password, this)) {
		warn("Error connecting to server");
		// Shutdown websocket connection and close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				output,
				"There was a problem connecting to your room.");
			obs_output_signal_stop(output,
					       OBS_OUTPUT_CONNECT_FAILED);
		});
		thread.detach();
		return false;
	}
	return true;
}

void WebRTCStream::onConnected()
{
	info("WebRTCStream::onConnected");
}

void WebRTCStream::onLogged(int /* code */)
{
	info("WebRTCStream::onLogged\nCreating offer...");
	webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offer_options;
	offer_options.voice_activity_detection = false;
	pc->CreateOffer(this, offer_options);
}

void WebRTCStream::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
	info("WebRTCStream::OnSuccess\n");
	std::string sdp;
	desc->ToString(&sdp);

	info("Audio codec:      %s", audio_codec.c_str());
	info("Audio bitrate:    %d kbps\n", audio_bitrate_);
	if (getVideoSourceCount() > 0) {
		info("Video codec:      %s",
		     video_codec.empty() ? "Automatic" : video_codec.c_str());
		info("Video bitrate:    %d kbps\n", video_bitrate_);
	} else {
		info("No video");
	}
	info("OFFER:\n\n%s\n", sdp.c_str());

	std::string sdpCopy = sdp;
	std::vector<int> audio_payloads;
	std::vector<int> video_payloads;
	// #271 do not list video codec H264 if it is not available in libwebrtc
	// If codec setting is Automatic, set it to h264 by default if available, else set it to VP9
	if (video_codec.empty()) {
#ifdef DISABLE_WEBRTC_H264
		video_codec = "VP9";
#else
		video_codec = "h264"; // h264 must be in lowercase (Firefox)
#endif
	}
	// Force specific video/audio payload
	SDPModif::forcePayload(sdpCopy, audio_payloads, video_payloads,
			       // the packaging mode needs to be 1
			       audio_codec, video_codec, 1, "42e01f", profile_);
	// Constrain video bitrate
	SDPModif::bitrateMaxMinSDP(sdpCopy, video_bitrate_, video_payloads);
	// Enable stereo & constrain audio bitrate
	// NOTE ALEX: check that it does not incorrectly detect multiopus as opus
	SDPModif::stereoSDP(sdpCopy, audio_bitrate_);
	// NOTE ALEX: nothing special to do about multiopus with CoSMo libwebrtc package.

	info("SETTING LOCAL DESCRIPTION\n\n");
	pc->SetLocalDescription(this, desc);

	info("Sending OFFER (SDP) to remote peer:\n\n%s", sdpCopy.c_str());
	if (!client->open(sdpCopy,
			  (0 == getVideoSourceCount() ? "" : video_codec),
			  audio_codec, username, multisource_, sourceId_)) {
		// Shutdown websocket connection and close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
		});
		thread.detach();
	}
}

void WebRTCStream::OnSuccess()
{
	info("Local Description set\n");
}

void WebRTCStream::OnFailure(webrtc::RTCError error)
{
	warn("WebRTCStream::OnFailure [%s]", error.message());
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
{
	std::string str;
	candidate->ToString(&str);
	// Send candidate to remote peer
	client->trickle(candidate->sdp_mid(), candidate->sdp_mline_index(), str,
			false);
}

void WebRTCStream::OnIceConnectionChange(
	webrtc::PeerConnectionInterface::IceConnectionState
		state /* new_state */)
{
	using namespace webrtc;
	info("WebRTCStream::OnIceConnectionChange [%u]", state);

	switch (state) {
	case PeerConnectionInterface::IceConnectionState::kIceConnectionFailed: {
		// Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_set_last_error(
				output,
				"We found your room, but streaming failed. Are you behind a firewall?\n\n");
			// Disconnect, this will call stop on main thread
			// #298 Close must be carried out on a separate thread in order to avoid deadlock
			auto thread = std::thread([=]() {
				obs_output_signal_stop(output,
						       OBS_OUTPUT_ERROR);
			});
			thread.detach();
		});
		thread.detach();
		break;
	}
	default:
		break;
	}
}

void WebRTCStream::OnConnectionChange(
	webrtc::PeerConnectionInterface::PeerConnectionState state)
{
	using namespace webrtc;
	info("WebRTCStream::OnConnectionChange [%u]", state);

	switch (state) {
	case PeerConnectionInterface::PeerConnectionState::kFailed: {

		// Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_set_last_error(output,
						  "Connection failure\n\n");
			// Disconnect, this will call stop on main thread
			// #298 Close must be carried out on a separate thread in order to avoid deadlock
			auto thread = std::thread([=]() {
				obs_output_signal_stop(output,
						       OBS_OUTPUT_ERROR);
			});
			thread.detach();
		});
		//Detach
		thread.detach();
		break;
	}
	default:
		break;
	}
}

void WebRTCStream::onRemoteIceCandidate(const std::string &sdpData)
{
	if (sdpData.empty()) {
		info("ICE COMPLETE\n");
		pc->AddIceCandidate(nullptr);
	} else {
		std::string s = sdpData;
		s.erase(remove(s.begin(), s.end(), '\"'), s.end());
		if (protocol.empty() ||
		    SDPModif::filterIceCandidates(s, protocol)) {
			const std::string candidate = s;
			info("Remote %s\n", candidate.c_str());
			const std::string sdpMid = "";
			int sdpMLineIndex = 0;
			webrtc::SdpParseError error;
			const webrtc::IceCandidateInterface *newCandidate =
				webrtc::CreateIceCandidate(sdpMid,
							   sdpMLineIndex,
							   candidate, &error);
			pc->AddIceCandidate(newCandidate);
		} else {
			info("Ignoring remote %s\n", s.c_str());
		}
	}
}

void WebRTCStream::onOpened(const std::string &sdp)
{
	info("ANSWER:\n\n%s\n", sdp.c_str());

	std::string sdpCopy = sdp;

	if (getVideoSourceCount() > 0) {
		// Constrain video bitrate
		SDPModif::bitrateSDP(sdpCopy, video_bitrate_);
	}
	// Enable stereo & constrain audio bitrate
	SDPModif::stereoSDP(sdpCopy, audio_bitrate_);

	// SetRemoteDescription observer
	srd_observer = make_scoped_refptr(this);

	// Create ANSWER from sdpCopy
	webrtc::SdpParseError error;
	std::unique_ptr<webrtc::SessionDescriptionInterface> answer =
		webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer,
						 sdpCopy, &error);

	info("SETTING REMOTE DESCRIPTION\n\n%s", sdpCopy.c_str());
	pc->SetRemoteDescription(std::move(answer), srd_observer);

	// Set audio conversion info
	audio_convert_info conversion;
	conversion.format = AUDIO_FORMAT_16BIT;
	conversion.samples_per_sec = audio_samplerate_;
	conversion.speakers = (speaker_layout)channel_count;
	obs_output_set_audio_conversion(output, &conversion);

	// Set video conversion info
	struct video_scale_info scaler = {};
	video_t *video = obs_output_video(output);
	const struct video_output_info *voi = video_output_get_info(video);
	scaler.format = voi->format;
	scaler.width = voi->width;
	scaler.height = voi->height;
	scaler.colorspace = voi->colorspace;
	scaler.range = voi->range;
	obs_output_set_video_conversion(output, &scaler);

	info("Begin data capture...");
	obs_output_begin_data_capture(output, 0);
}

void WebRTCStream::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
	if (error.ok())
		info("Remote Description set\n");
	else {
		warn("Error setting Remote Description: %s\n", error.message());
		// Shutdown websocket connection and close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
		});
		thread.detach();
	}
}

bool WebRTCStream::close(bool wait)
{
	if (!pc.get())
		return false;
	// Get pointer
	auto old = pc.release();
	// Close Peer Connection
	old->Close();
	// Shutdown websocket connection
	if (client) {
		client->disconnect(wait);
		delete (client);
		client = nullptr;
	}
	return true;
}

bool WebRTCStream::stop()
{
	info("WebRTCStream::stop");
	// Shutdown websocket connection and close Peer Connection
	close(true);
	// Disconnect, this will call stop on main thread
	obs_output_end_data_capture(output);

	// Empty video queue
	std::unique_lock<std::mutex> lock(mutex_video_queue_);
	while (!video_queue_.empty()) {
		video_data *frame = video_queue_.front();
		video_queue_.pop();
		// Free up memory
		free(frame->data[0]);
		free(frame);
	}
	lock.unlock();

	return true;
}

void WebRTCStream::onDisconnected()
{
	info("WebRTCStream::onDisconnected");

	// are we done retrying?
	if (thread_closeAsync.joinable())
		thread_closeAsync.join();

	// Shutdown websocket connection and close Peer Connection asynchronously
	thread_closeAsync = std::thread([&]() {
		close(false);
		// Disconnect, this will call stop on main thread
		// #298 Close must be carried out on a separate thread in order to avoid deadlock
		auto thread = std::thread([=]() {
			obs_output_signal_stop(output, OBS_OUTPUT_DISCONNECTED);
		});
		thread.detach();
	});
}

void WebRTCStream::onLoggedError(int code)
{
	info("WebRTCStream::onLoggedError [code: %d]", code);
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	// #298 Close must be carried out on a separate thread in order to avoid deadlock
	auto thread = std::thread([=]() {
		obs_output_set_last_error(
			output,
			"We are having trouble connecting to your room.  Are you behind a firewall?\n");
		obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
	});
	thread.detach();
}

void WebRTCStream::onOpenedError(int code)
{
	info("WebRTCStream::onOpenedError [code: %d]", code);
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	// #298 Close must be carried out on a separate thread in order to avoid deadlock
	auto thread = std::thread(
		[=]() { obs_output_signal_stop(output, OBS_OUTPUT_ERROR); });
	thread.detach();
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
	if (!frame)
		return;

	if (getVideoSourceCount() == 0) {
		// No video, no synchronisation needed with video
		// Push frame to the device
		audio_source->OnAudioData(frame);
		return;
	}

	if (!audio_started_) {
		audio_started_ = true;
	}

	audio_source->OnAudioData(frame);
	last_delivered_audio_ts_ = frame->timestamp;
	process_video_queue();
}

void WebRTCStream::onVideoFrame(video_data *frame)
{
	if (!frame)
		return;
	if (!videoCapturer)
		return;

	if (!first_frame_received_) {
		// Initialize color information from first frame
		first_frame_received_ = true;

		if ("I420" == colorFormat_) {
			videoType_ = webrtc::VideoType::kI420;
		} else if ("I444" == colorFormat_) {
			videoType_ = webrtc::VideoType::kI444;
		} else if ("I010" == colorFormat_) {
			videoType_ = webrtc::VideoType::kI010;
		} else {
			videoType_ = webrtc::VideoType::kNV12;
		}

		video_t *video = obs_output_video(output);
		const struct video_output_info *voi = video_output_get_info(video);

		// Video format
		switch (voi->format) {
		case VIDEO_FORMAT_I420:
			if ("I420" == colorFormat_)
				videoType_ = webrtc::VideoType::kI420;
			else
				info("ERROR: Color format selected is %s, but video frame received is of format %s",
					colorFormat_.c_str(), get_video_format_name(voi->format));
			break;
		case VIDEO_FORMAT_NV12:
			if ("NV12" == colorFormat_)
				videoType_ = webrtc::VideoType::kNV12;
			else
				info("ERROR: Color format selected is %s, but video frame received is of format %s",
					colorFormat_.c_str(), get_video_format_name(voi->format));
			break;
		case VIDEO_FORMAT_I444:
			if ("I444" == colorFormat_)
				videoType_ = webrtc::VideoType::kI444;
			else
				info("ERROR: Color format selected is %s, but video frame received is of format %s",
					colorFormat_.c_str(), get_video_format_name(voi->format));
			break;
		case VIDEO_FORMAT_I010:
			if ("I010" == colorFormat_) {
				videoType_ = webrtc::VideoType::kI010;
			}
			else
				info("ERROR: Color format selected is %s, but video frame received is of format %s",
					colorFormat_.c_str(), get_video_format_name(voi->format));
			break;
		default:
			info("ERROR: Color format selected is %s, this format is not supported for WebRTC streaming",
				get_video_format_name(voi->format));
		}

		// Color space
		switch (voi->colorspace) {
		case VIDEO_CS_601:
			color_space_.set_primaries_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::PrimaryID::kSMPTE170M));
			color_space_.set_transfer_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::TransferID::kSMPTE170M));
			color_space_.set_matrix_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::MatrixID::kSMPTE170M));
			break;
		case VIDEO_CS_DEFAULT:
		case VIDEO_CS_709:
			color_space_.set_primaries_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::PrimaryID::kBT709));
			color_space_.set_transfer_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::TransferID::kBT709));
			color_space_.set_matrix_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::MatrixID::kBT709));
			break;
		case VIDEO_CS_SRGB:
			color_space_.set_primaries_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::PrimaryID::kBT709));
			color_space_.set_transfer_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::TransferID::kIEC61966_2_1));
			color_space_.set_matrix_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::MatrixID::kRGB));
			break;
		case VIDEO_CS_2100_PQ:
			color_space_.set_primaries_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::PrimaryID::kBT2020));
			color_space_.set_transfer_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::TransferID::kSMPTEST2084));
			color_space_.set_matrix_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::MatrixID::kBT2020_NCL)); // non-constant luminance system
			hdrmetadata_.mastering_metadata.luminance_max = obs_get_video_hdr_nominal_peak_level();
			hdrmetadata_.mastering_metadata.luminance_min = obs_get_video_sdr_white_level();
			color_space_.set_hdr_metadata(&hdrmetadata_);
			break;
		case VIDEO_CS_2100_HLG:
			color_space_.set_primaries_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::PrimaryID::kBT2020));
			color_space_.set_transfer_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::TransferID::kARIB_STD_B67));
			color_space_.set_matrix_from_uint8(
				static_cast<uint8_t>(webrtc::ColorSpace::MatrixID::kBT2020_NCL)); // non-constant luminance system
			hdrmetadata_.mastering_metadata.luminance_max = obs_get_video_hdr_nominal_peak_level();
			hdrmetadata_.mastering_metadata.luminance_min = obs_get_video_sdr_white_level();
			color_space_.set_hdr_metadata(&hdrmetadata_);
			break;
		}

		// Color range
		color_space_.set_range_from_uint8(
			voi->range == VIDEO_RANGE_FULL
			? static_cast<uint8_t>(webrtc::ColorSpace::RangeID::kFull)
			: static_cast<uint8_t>(webrtc::ColorSpace::RangeID::kLimited));
	}

	if (!audio_started_) {
		enqueue_frame(frame);
		return;
	}

	// Handle synchronisation with audio
	if (frame->timestamp <= last_delivered_audio_ts_) {
		std::unique_lock<std::mutex> lock(mutex_deliver_video_frame_);
		deliver_video_frame(frame);
		lock.unlock();
	} else {
		enqueue_frame(frame);
	}
}

void WebRTCStream::enqueue_frame(video_data *frame) {
	video_data *framecopy = (video_data*)malloc(sizeof(video_data));
	framecopy->timestamp = frame->timestamp;

	int outputWidth = obs_output_get_width(output);
	int outputHeight = obs_output_get_height(output);
	uint32_t size = 0;
	if ("NV12" == colorFormat_ || "I420" == colorFormat_) {
		size = outputWidth * outputHeight * 3 / 2;
	} else if ("I444" == colorFormat_ || "RGB" == colorFormat_) {
		size = outputWidth * outputHeight * 3;
	} else if ("I010" == colorFormat_) {
		size = 2 * outputWidth * outputHeight * 3 / 2;
	}
	framecopy->data[0] = (uint8_t *)malloc(size * sizeof(uint8_t));
	memcpy(framecopy->data[0], frame->data[0], size * sizeof(uint8_t));
	framecopy->linesize[0] = frame->linesize[0];

	for (size_t i=1; i < MAX_AV_PLANES; ++i) {
		if (frame->linesize[i] != 0) {
			framecopy->linesize[i] = frame->linesize[i];
			if ("NV12" == colorFormat_ || "I420" == colorFormat_ || "I010" == colorFormat_) {
				framecopy->data[i] = framecopy->data[i-1] + framecopy->linesize[i-1];
			} else if ("I444" == colorFormat_ || "RGB" == colorFormat_) {
				framecopy->data[i] = framecopy->data[i-1] + outputWidth * outputHeight;
			}
		} else {
			framecopy->linesize[i] = 0;
			framecopy->data[i] = NULL;
		}
	}

	std::unique_lock<std::mutex> lock(mutex_video_queue_);
	video_queue_.push(framecopy);
	lock.unlock();
}

void WebRTCStream::process_video_queue()
{
	std::unique_lock<std::mutex> lock_queue(mutex_video_queue_);
	while (!video_queue_.empty()) {
		video_data *frame = video_queue_.front();
		if (frame->timestamp <= last_delivered_audio_ts_) {
			std::unique_lock<std::mutex> lock_frame(mutex_deliver_video_frame_);
			deliver_video_frame(frame);
			// Free up memory
			free(frame->data[0]);
			free(frame);
			lock_frame.unlock();
			video_queue_.pop();
		} else {
			break;
		}
	}
	lock_queue.unlock();
}

void WebRTCStream::deliver_video_frame(video_data *frame) {
	if (std::chrono::system_clock::time_point(
		    std::chrono::duration<int>(0)) == previous_time_) {
		// First frame sent: Initialize previous_time
		previous_time_ = std::chrono::system_clock::now();
	}

	// Calculate size
	int outputWidth = obs_output_get_width(output);
	int outputHeight = obs_output_get_height(output);
	int target_width = abs(outputWidth);
	int target_height = abs(outputHeight);

	// Align timestamps from OBS capturer with rtc::TimeMicros timebase
	const int64_t obs_timestamp_us = (int64_t)frame->timestamp /
						rtc::kNumNanosecsPerMicrosec;
	const int64_t aligned_timestamp_us =
		timestamp_aligner_.TranslateTimestamp(
			obs_timestamp_us, rtc::TimeMicros());

	if ("NV12" == colorFormat_ || "I420" == colorFormat_) {
		rtc::scoped_refptr<webrtc::I420Buffer> buffer =
			webrtc::I420Buffer::Create(target_width, target_height);

		// Convert frame
		uint32_t size = outputWidth * outputHeight * 3 / 2;
		libyuv::RotationMode rotation_mode = libyuv::kRotate0;
		const int conversionResult = libyuv::ConvertToI420(
			frame->data[0], size,
			buffer.get()->MutableDataY(), buffer.get()->StrideY(),
			buffer.get()->MutableDataU(), buffer.get()->StrideU(),
			buffer.get()->MutableDataV(), buffer.get()->StrideV(),
			0, 0, outputWidth, outputHeight, target_width, target_height,
			rotation_mode, ConvertVideoType(videoType_));
		// not using the result yet, silence compiler
		(void)conversionResult;

		// Create a webrtc::VideoFrame to pass to the capturer
		webrtc::VideoFrame video_frame =
			webrtc::VideoFrame::Builder()
				.set_video_frame_buffer(buffer)
				.set_color_space(color_space_)
				.set_rotation(webrtc::kVideoRotation_0)
				.set_timestamp_us(aligned_timestamp_us)
				.set_id(++frame_id_)
				.build();

		// Send frame to video capturer
		videoCapturer->OnFrameCaptured(video_frame);

	} else if ("I444" == colorFormat_) {
		rtc::scoped_refptr<webrtc::I444Buffer> buffer444 =
			webrtc::I444Buffer::Create(target_width, target_height);

		// Copy OBS frame to webrtc i444 buffer
		uint8_t *datay = const_cast<uint8_t *>(buffer444->DataY());
		memcpy(datay, frame->data[0], frame->linesize[0] * outputHeight);
		uint8_t *datau = const_cast<uint8_t *>(buffer444->DataU());
		memcpy(datau, frame->data[1], frame->linesize[1] * outputHeight);
		uint8_t *datav = const_cast<uint8_t *>(buffer444->DataV());
		memcpy(datav, frame->data[2], frame->linesize[2] * outputHeight);

		// Create a webrtc::VideoFrame to pass to the capturer
		webrtc::VideoFrame video_frame444 =
			webrtc::VideoFrame::Builder()
				.set_video_frame_buffer(buffer444)
				.set_color_space(color_space_)
				.set_rotation(webrtc::kVideoRotation_0)
				.set_timestamp_us(aligned_timestamp_us)
				.set_id(++frame_id_)
				.build();

		// Send frame to video capturer
		videoCapturer->OnFrameCaptured(video_frame444);

	} else if ("I010" == colorFormat_) {
		rtc::scoped_refptr<webrtc::I010Buffer> buffer010 =
			webrtc::I010Buffer::Create(outputWidth, outputHeight);

		// Convert frame
		uint32_t size = outputWidth * outputHeight * 3 / 2;
		libyuv::RotationMode rotation_mode = libyuv::kRotate0;
		const int conversionResult = libyuv::ConvertToI010(
			reinterpret_cast<const uint16_t*>(frame->data[0]), size,
			buffer010.get()->MutableDataY(), buffer010.get()->StrideY(),
			buffer010.get()->MutableDataU(), buffer010.get()->StrideU(),
			buffer010.get()->MutableDataV(), buffer010.get()->StrideV(),
			0, 0, outputWidth,outputHeight, target_width, target_height,
			rotation_mode, ConvertVideoType(videoType_));
		// not using the result yet, silence compiler
		(void)conversionResult;

		// Create a webrtc::VideoFrame to pass to the capturer
		webrtc::VideoFrame video_frame010 =
			webrtc::VideoFrame::Builder()
				.set_video_frame_buffer(buffer010)
				.set_color_space(color_space_)
				.set_rotation(webrtc::kVideoRotation_0)
				.set_timestamp_us(aligned_timestamp_us)
				.set_id(++frame_id_)
				.build();

		// Send frame to video capturer
		videoCapturer->OnFrameCaptured(video_frame010);
	}
}

// Count number of video sources in current scene
int WebRTCStream::getVideoSourceCount() const
{
	auto countSources = [](void *param, obs_source_t *source) {
		if (!source)
			return true;

		uint32_t flags = obs_source_get_output_flags(source);
		if ((flags & OBS_SOURCE_VIDEO) != 0)
			(*reinterpret_cast<int *>(param))++;

		return true;
	};
	int count_video_source = 0;
	obs_enum_sources(countSources, &count_video_source);
	return count_video_source;
}

// NOTE LUDO: #80 add getStats
void WebRTCStream::getStats()
{
	auto reports = NewGetStats();
	if (reports.empty()) {
		return;
	}

	// FPS = frames_sent / (time_now - start_time)
	auto time_now = std::chrono::system_clock::now();
	auto elapsed_seconds = time_now - previous_time_;
	previous_time_ = time_now;

	stats_list_ = "";
	total_bytes_sent_ = 0;
	uint32_t data_messages_sent = 0;
	uint64_t data_bytes_sent = 0;
	uint32_t data_messages_received = 0;
	uint64_t data_bytes_received = 0;

	transport_bytes_sent_ = 0;
	transport_bytes_received_ = 0;
	video_packets_sent_ = 0;
	video_bytes_sent_ = 0;
	pli_received_ = 0;
	video_fir_count_ = 0;
	video_nack_count_ = 0;
	video_qp_sum_ = 0;
	audio_packets_sent_ = 0;
	audio_bytes_sent_ = 0;
	track_audio_level_ = 0;
	track_total_audio_energy_ = 0;
	track_total_samples_duration_ = 0;
	track_frame_width_ = 0;
	track_frame_height_ = 0;
	track_frames_sent_ = 0;
	track_huge_frames_sent_ = 0;

	for (const auto &report : reports) {
		// RTCDataChannelStats
		std::vector<const webrtc::RTCDataChannelStats *>
			data_channel_stats = report->GetStatsOfType<
				webrtc::RTCDataChannelStats>();
		for (const auto &stat : data_channel_stats) {
			data_messages_sent +=
				std::stoi(stat->messages_sent.ValueToJson());
			data_bytes_sent +=
				std::stoll(stat->bytes_sent.ValueToJson());
			data_messages_received += std::stoi(
				stat->messages_received.ValueToJson());
			data_bytes_received +=
				std::stoll(stat->bytes_received.ValueToJson());
		}

		// RTCMediaStreamTrackStats
		// double audio_track_jitter_buffer_delay = 0.0;
		// double video_track_jitter_buffer_delay = 0.0;
		// uint64_t audio_track_jitter_buffer_emitted_count = 0;
		// uint64_t video_track_jitter_buffer_emitted_count = 0;
		std::vector<const webrtc::RTCMediaStreamTrackStats *>
			media_stream_track_stats = report->GetStatsOfType<
				webrtc::RTCMediaStreamTrackStats>();
		for (const auto &stat : media_stream_track_stats) {
			if (stat->kind.ValueToString() == "audio") {
				if (stat->audio_level.is_defined()) {
					track_audio_level_ += std::stoi(
						stat->audio_level.ValueToJson());
					stats_list_ +=
						"track_audio_level:" +
						stat->audio_level.ValueToJson() +
						"\n";
				}
				if (stat->total_audio_energy.is_defined()) {
					track_total_audio_energy_ += std::stoi(
						stat->total_audio_energy
							.ValueToJson());
					stats_list_ +=
						"track_total_audio_energy:" +
						stat->total_audio_energy
							.ValueToJson() +
						"\n";
				}
				// stats_list += "track_echo_return_loss:"   + stat->echo_return_loss.ValueToJson() + "\n";
				// stats_list += "track_echo_return_loss_enhancement:" + stat->echo_return_loss_enhancement.ValueToJson() + "\n";
				// stats_list += "track_total_samples_received:" + stat->total_samples_received.ValueToJson() + "\n";
				if (stat->total_samples_duration.is_defined()) {
					track_total_samples_duration_ +=
						std::stoi(
							stat->total_samples_duration
								.ValueToJson());
					stats_list_ +=
						"track_total_samples_duration:" +
						stat->total_samples_duration
							.ValueToJson() +
						"\n";
				}
				// stats_list += "track_concealed_samples:" + stat->concealed_samples.ValueToJson() + "\n";
				// stats_list += "track_concealment_events:" + stat->concealment_events.ValueToJson() + "\n";
			}
			if (stat->kind.ValueToString() == "video") {
				// stats_list += "track_jitter_buffer_delay:"    + stat->jitter_buffer_delay.ValueToJson() + "\n";
				// stats_list += "track_jitter_buffer_emitted_count:" + stat->jitter_buffer_emitted_count.ValueToJson() + "\n";
				track_frame_width_ = std::stoi(
					stat->frame_width.ValueToJson());
				stats_list_ += "track_frame_width:" +
					       stat->frame_width.ValueToJson() +
					       "\n";
				track_frame_height_ = std::stoi(
					stat->frame_height.ValueToJson());
				stats_list_ +=
					"track_frame_height:" +
					stat->frame_height.ValueToJson() + "\n";
				track_frames_sent_ += std::stoll(
					stat->frames_sent.ValueToJson());
				stats_list_ += "track_frames_sent:" +
					       stat->frames_sent.ValueToJson() +
					       "\n";
				track_huge_frames_sent_ += std::stoll(
					stat->huge_frames_sent.ValueToJson());
				stats_list_ +=
					"track_huge_frames_sent:" +
					stat->huge_frames_sent.ValueToJson() +
					"\n";

				// FPS = frames_sent / (time_now - start_time)
				uint32_t frames_sent = std::stol(
					stat->frames_sent.ValueToJson());
				stats_list_ +=
					"track_fps:" +
					std::to_string(
						(frames_sent -
						 previous_frames_sent_) /
						elapsed_seconds.count()) +
					"\n";
				previous_frames_sent_ = frames_sent;

				// stats_list += "track_frames_received:"        + stat->frames_received.ValueToJson() + "\n";
				// stats_list += "track_frames_decoded:"         + stat->frames_decoded.ValueToJson() + "\n";
				// stats_list += "track_frames_dropped:"         + stat->frames_dropped.ValueToJson() + "\n";
				// stats_list += "track_frames_corrupted:"       + stat->frames_corrupted.ValueToJson() + "\n";
				// stats_list += "track_partial_frames_lost:"    + stat->partial_frames_lost.ValueToJson() + "\n";
				// stats_list += "track_full_frames_lost:"       + stat->full_frames_lost.ValueToJson() + "\n";
			}
		}

		// RTCOutboundRTPStreamStats
		std::vector<const webrtc::RTCOutboundRTPStreamStats *>
			outbound_stream_stats = report->GetStatsOfType<
				webrtc::RTCOutboundRTPStreamStats>();
		for (const auto &stat : outbound_stream_stats) {
			if (stat->kind.ValueToString() == "audio") {
				audio_packets_sent_ += std::stoll(
					stat->packets_sent.ValueToJson());
				stats_list_ +=
					"outbound_audio_packets_sent:" +
					stat->packets_sent.ValueToJson() + "\n";
				audio_bytes_sent_ += std::stoll(
					stat->bytes_sent.ValueToJson());
				stats_list_ += "outbound_audio_bytes_sent:" +
					       stat->bytes_sent.ValueToJson() +
					       "\n";
				// stats_list += "outbound_audio_target_bitrate:" + stat->target_bitrate.ValueToJson() + "\n";
				// stats_list += "outbound_audio_frames_encoded:" + stat->frames_encoded.ValueToJson() + "\n";
			}
			if (stat->kind.ValueToString() == "video") {
				video_packets_sent_ += std::stoll(
					stat->packets_sent.ValueToJson());
				stats_list_ +=
					"outbound_video_packets_sent:" +
					stat->packets_sent.ValueToJson() + "\n";
				video_bytes_sent_ += std::stoll(
					stat->bytes_sent.ValueToJson());
				stats_list_ += "outbound_video_bytes_sent:" +
					       stat->bytes_sent.ValueToJson() +
					       "\n";
				// stats_list += "outbound_video_target_bitrate:" + stat->target_bitrate.ValueToJson() + "\n";
				// stats_list += "outbound_video_frames_encoded:" + stat->frames_encoded.ValueToJson() + "\n";
				video_fir_count_ += std::stoll(
					stat->fir_count.ValueToJson());
				stats_list_ += "outbound_video_fir_count:" +
					       stat->fir_count.ValueToJson() +
					       "\n";
				pli_received_ += std::stoi(
					stat->pli_count.ValueToJson());
				stats_list_ += "outbound_video_pli_count:" +
					       stat->pli_count.ValueToJson() +
					       "\n";
				video_nack_count_ += std::stoll(
					stat->nack_count.ValueToJson());
				stats_list_ += "outbound_video_nack_count:" +
					       stat->nack_count.ValueToJson() +
					       "\n";
				// stats_list += "outbound_video_sli_count:"      + stat->sli_count.ValueToJson() + "\n";
				if (stat->qp_sum.is_defined()) {
					video_qp_sum_ += std::stoll(
						stat->qp_sum.ValueToJson());
					stats_list_ +=
						"outbound_video_qp_sum:" +
						stat->qp_sum.ValueToJson() +
						"\n";
				}
			}
		}

		// RTCTransportStats
		std::vector<const webrtc::RTCTransportStats *> transport_stats =
			report->GetStatsOfType<webrtc::RTCTransportStats>();
		for (const auto &stat : transport_stats) {
			transport_bytes_sent_ =
				std::stoll(stat->bytes_sent.ValueToJson());
			stats_list_ += "transport_bytes_sent:" +
				       stat->bytes_sent.ValueToJson() + "\n";
			transport_bytes_received_ =
				std::stoll(stat->bytes_received.ValueToJson());
			stats_list_ += "transport_bytes_received:" +
				       stat->bytes_received.ValueToJson() +
				       "\n";
		}

	} // loop on reports

	total_bytes_sent_ = audio_bytes_sent_ + video_bytes_sent_;

	// RTCDataChannelStats
	stats_list_ += "data_messages_sent:" +
		       std::to_string(data_messages_sent) + "\n";
	stats_list_ +=
		"data_bytes_sent:" + std::to_string(data_bytes_sent) + "\n";
	stats_list_ += "data_messages_received:" +
		       std::to_string(data_messages_received) + "\n";
	stats_list_ += "data_bytes_received:" +
		       std::to_string(data_bytes_received) + "\n";

	// RTCPeerConnectionStats
	// std::vector<const webrtc::RTCPeerConnectionStats*> pc_stats =
	//         report->GetStatsOfType<webrtc::RTCPeerConnectionStats>();
	// for (const auto& stat : pc_stats) {
	//   stats_list += "data_channels_opened:" + stat->data_channels_opened.ValueToJson() + "\n";
	//   stats_list += "data_channels_closed:" + stat->data_channels_closed.ValueToJson() + "\n";
	// }

	// RTCRTPStreamStats
	// std::vector<const webrtc::RTCRTPStreamStats*> rtcrtp_stats =
	//         report->GetStatsOfType<webrtc::RTCRTPStreamStats>();
	// for (const auto& stat : rtcrtp_stats) {
	//   if (stat->kind.ValueToString() == "video") {
	//     stats_list += "rtcrtp_fir_count:" + stat->fir_count.ValueToJson() + "\n";
	//     stats_list += "rtcrtp_pli_count:" + stat->pli_count.ValueToJson() + "\n";
	//     stats_list += "rtcrtp_nack_count:" + stat->nack_count.ValueToJson() + "\n";
	//     stats_list += "rtcrtp_sli_count:" + stat->sli_count.ValueToJson() + "\n";
	//     stats_list += "rtcrtp_qp_sum:" + stat->qp_sum.ValueToJson() + "\n";
	//   }
	// }

	// RTCInboundRTPStreamStats
	// std::vector<const webrtc::RTCInboundRTPStreamStats*> inbound_stream_stats =
	//         report->GetStatsOfType<webrtc::RTCInboundRTPStreamStats>();
	// for (const auto& stat : inbound_stream_stats) {
	//   stats_list += "inbound_stream_packets_received:"        + stat->packets_received.ValueToJson() + "\n";
	//   stats_list += "inbound_stream_bytes_received:"          + stat->bytes_received.ValueToJson() + "\n";
	//   stats_list += "inbound_stream_packets_lost:"            + stat->packets_lost.ValueToJson() + "\n";

	//   if (stat->kind.ValueToString() == "audio") {
	//     stats_list += "inbound_stream_jitter:" + stat->jitter.ValueToJson() + "\n";
	//   }
	// }
}

std::vector<rtc::scoped_refptr<const webrtc::RTCStatsReport>>
WebRTCStream::NewGetStats()
{
	webrtc::MutexLock lock(&crit_);

	if (nullptr == pc) {
		return {};
	}

	std::vector<rtc::scoped_refptr<StatsCallback>> vector_stats_callbacks;
	for (const auto &transceiver : pc->GetTransceivers()) {
		auto sender = transceiver->sender();
		rtc::scoped_refptr<StatsCallback> stats_callback(
			new rtc::RefCountedObject<StatsCallback>());
		pc->GetStats(sender, stats_callback);
		vector_stats_callbacks.push_back(stats_callback);
	}

	std::vector<rtc::scoped_refptr<const webrtc::RTCStatsReport>> reports;
	for (const auto &stats_callback : vector_stats_callbacks) {
		while (!stats_callback->called()) {
			std::this_thread::sleep_for(
				std::chrono::microseconds(1));
		}
		reports.push_back(stats_callback->report());
	}

	return reports;
}
