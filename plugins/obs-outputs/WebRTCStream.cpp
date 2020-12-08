// Copyright Dr. Alex. Gouaillard (2015, 2020)

#include "WebRTCStream.h"
#include "SDPModif.h"

#include "media-io/video-io.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video/i420_buffer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "pc/rtc_stats_collector.h"
#include "rtc_base/checks.h"
#include "rtc_base/critical_section.h"
#include <libyuv.h>

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

	audio_bitrate = 128;
	video_bitrate = 2500;

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

	factory = webrtc::CreatePeerConnectionFactory(
		network.get(), worker.get(), signaling.get(), adm,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		webrtc::CreateBuiltinAudioDecoderFactory(),
		webrtc::CreateBuiltinVideoEncoderFactory(),
		webrtc::CreateBuiltinVideoDecoderFactory(), nullptr, nullptr);

	// Create video capture module
	videoCapturer = new rtc::RefCountedObject<VideoCapturer>();
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
	stats_list = "";
	frame_id = 0;
	pli_received = 0;
	audio_bytes_sent = 0;
	video_bytes_sent = 0;
	total_bytes_sent = 0;
	previous_frames_sent = 0;
}

bool WebRTCStream::start(WebRTCStream::Type type)
{
	info("WebRTCStream::start");
	this->type = type;

	resetStats();

	// Access service if started, or fail

	obs_service_t *service = obs_output_get_service(output);
	if (!service) {
		obs_output_set_last_error(
			output,
			"An unexpected error occurred during stream startup.");
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	// Extract setting from service

	url = obs_service_get_url(service) ? obs_service_get_url(service) : "";
	room = obs_service_get_room(service) ? obs_service_get_room(service)
					     : "";
	username = obs_service_get_username(service)
			   ? obs_service_get_username(service)
			   : "";
	password = obs_service_get_password(service)
			   ? obs_service_get_password(service)
			   : "";
	video_codec = obs_service_get_codec(service)
			      ? obs_service_get_codec(service)
			      : "";
	protocol = obs_service_get_protocol(service)
			   ? obs_service_get_protocol(service)
			   : "";
	simulcast = obs_service_get_simulcast(service)
			    ? obs_service_get_simulcast(service)
			    : false;
	publishApiUrl = obs_service_get_publishApiUrl(service)
				? obs_service_get_publishApiUrl(service)
				: "";

	// Some extra log

	info("Video codec: %s",
	     video_codec.empty() ? "Automatic" : video_codec.c_str());
	info("Simulcast: %s", simulcast ? "true" : "false");
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
		obs_output_set_last_error(
			output,
			"Your service settings are not complete. Open the settings => stream window and complete them.");
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	// Set up encoders.
	// NOTE ALEX: should not be done for webrtc.

	obs_output_t *context = output;

	obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
	obs_data_t *asettings = obs_encoder_get_settings(aencoder);
	audio_bitrate = (int)obs_data_get_int(asettings, "bitrate");
	obs_data_release(asettings);

	obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
	obs_data_t *vsettings = obs_encoder_get_settings(vencoder);
	video_bitrate = (int)obs_data_get_int(vsettings, "bitrate");
	obs_data_release(vsettings);

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
	if (close(false))
		obs_output_signal_stop(output, OBS_OUTPUT_ERROR);

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

	pc = factory->CreatePeerConnection(config, std::move(dependencies));

	if (!pc.get()) {
		error("Error creating Peer Connection");
		obs_output_set_last_error(
			output,
			"There was an error connecting to the server. Are you connected to the internet?");
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	} else {
		info("PEER CONNECTION CREATED\n");
	}

	cricket::AudioOptions options;
	options.echo_cancellation.emplace(false); // default: true
	options.auto_gain_control.emplace(false); // default: true
	options.noise_suppression.emplace(false); // default: true
	options.highpass_filter.emplace(false);   // default: true
	options.stereo_swapping.emplace(false);
	options.typing_detection.emplace(false); // default: true
	options.experimental_agc.emplace(false);
	// m79 options.extended_filter_aec.emplace(false);
	// m79 options.delay_agnostic_aec.emplace(false);
	options.experimental_ns.emplace(false);
	options.residual_echo_detector.emplace(false); // default: true
	// options.tx_agc_limiter.emplace(false);

	stream = factory->CreateLocalMediaStream("obs");

	audio_source = obsWebrtcAudioSource::Create(&options);
	audio_track = factory->CreateAudioTrack("audio", audio_source);
	// pc->AddTrack(audio_track, {"obs"});
	stream->AddTrack(audio_track);

	video_track = factory->CreateVideoTrack("video", videoCapturer);
	// pc->AddTrack(video_track, {"obs"});
	stream->AddTrack(video_track);

	//Add audio track
	webrtc::RtpTransceiverInit audio_init;
	audio_init.stream_ids.push_back(stream->id());
	audio_init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
	pc->AddTransceiver(audio_track, audio_init);

	bool simulcast = false;

	//Add video track
	webrtc::RtpTransceiverInit video_init;
	video_init.stream_ids.push_back(stream->id());
	video_init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
	if (simulcast) {
		webrtc::RtpEncodingParameters large;
		webrtc::RtpEncodingParameters medium;
		webrtc::RtpEncodingParameters small;
		large.rid = "L";
		large.scale_resolution_down_by = 1;
		medium.rid = "M";
		medium.scale_resolution_down_by = 2;
		small.rid = "S";
		small.scale_resolution_down_by = 4;
		//In reverse order so large is dropped first on low network condition
		video_init.send_encodings.push_back(small);
		video_init.send_encodings.push_back(medium);
		video_init.send_encodings.push_back(large);
	}
	pc->AddTransceiver(video_track, video_init);

	client = createWebsocketClient(type);
	if (!client) {
		warn("Error creating Websocket client");
		// Close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		obs_output_set_last_error(
			output,
			"There was a problem creating the websocket connection.  Are you behind a firewall?");
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	// Extra logging

	if (type == WebRTCStream::Type::Millicast) {
		info("Stream Name:      %s\nPublishing Token: %s\n",
		     username.c_str(), password.c_str());
		url = publishApiUrl;
	}
	info("CONNECTING TO %s", url.c_str());

	// Connect to the signalling server
	if (!client->connect(url, room, username, password, this)) {
		warn("Error connecting to server");
		// Shutdown websocket connection and close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		obs_output_set_last_error(
			output, "There was a problem connecting to your room.");
		obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
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
	info("Audio bitrate:    %d\n", audio_bitrate);
	info("Video codec:      %s",
	     video_codec.empty() ? "Automatic" : video_codec.c_str());
	info("Video bitrate:    %d\n", video_bitrate);
	info("OFFER:\n\n%s\n", sdp.c_str());

	std::string sdpCopy = sdp;
	std::vector<int> audio_payloads;
	std::vector<int> video_payloads;
	// If codec setting is Automatic, set it to h264 by default
	if (video_codec.empty()) {
		video_codec = "h264"; // h264 must be in lowercase (Firefox)
	}
	// Force specific video/audio payload
	SDPModif::forcePayload(sdpCopy, audio_payloads, video_payloads,
			       // the packaging mode needs to be 1
			       audio_codec, video_codec, 1, "42e01f", 0);
	// Constrain video bitrate
	SDPModif::bitrateMaxMinSDP(sdpCopy, video_bitrate, video_payloads);
	// Enable stereo & constrain audio bitrate
	// NOTE ALEX: check that it does not incorrectly detect multiopus as opus
	SDPModif::stereoSDP(sdpCopy, audio_bitrate);
	// NOTE ALEX: nothing special to do about multiopus with CoSMo libwebrtc package.

	info("SETTING LOCAL DESCRIPTION\n\n");
	pc->SetLocalDescription(this, desc);

	info("Sending OFFER (SDP) to remote peer:\n\n%s", sdpCopy.c_str());
	if (!client->open(sdpCopy, video_codec, audio_codec, username)) {
		// Shutdown websocket connection and close Peer Connection
		close(false);
		// Disconnect, this will call stop on main thread
		obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
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
			obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
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
			obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
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

	// Constrain video bitrate
	SDPModif::bitrateSDP(sdpCopy, video_bitrate);
	// Enable stereo & constrain audio bitrate
	SDPModif::stereoSDP(sdpCopy, audio_bitrate);

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
	conversion.samples_per_sec = 48000;
	conversion.speakers = (speaker_layout)channel_count;
	obs_output_set_audio_conversion(output, &conversion);

	info("Begin data capture...");
	obs_output_begin_data_capture(output, 0);
}

void WebRTCStream::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
	if (error.ok())
		info("Remote Description set\n");
	else
		warn("Error setting Remote Description: %s\n", error.message());
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
		obs_output_signal_stop(output, OBS_OUTPUT_DISCONNECTED);
	});
}

void WebRTCStream::onLoggedError(int code)
{
	info("WebRTCStream::onLoggedError [code: %d]", code);
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	obs_output_set_last_error(
		output,
		"We are having trouble connecting to your room. Are you behind a firewall?\n");
	obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onOpenedError(int code)
{
	info("WebRTCStream::onOpenedError [code: %d]", code);
	// Shutdown websocket connection and close Peer Connection
	close(false);
	// Disconnect, this will call stop on main thread
	obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
	if (!frame)
		return;
	// Push it to the device
	audio_source->OnAudioData(frame);
}

void WebRTCStream::onVideoFrame(video_data *frame)
{
	if (!frame)
		return;
	if (!videoCapturer)
		return;

	if (std::chrono::system_clock::time_point(
		    std::chrono::duration<int>(0)) == previous_time)
		// First frame sent: Initialize previous_time
		previous_time = std::chrono::system_clock::now();

	// Calculate size
	int outputWidth = obs_output_get_width(output);
	int outputHeight = obs_output_get_height(output);
	auto videoType = webrtc::VideoType::kNV12;
	uint32_t size = outputWidth * outputHeight * 3 / 2;

	int stride_y = outputWidth;
	int stride_uv = (outputWidth + 1) / 2;
	int target_width = abs(outputWidth);
	int target_height = abs(outputHeight);

	// Convert frame
	rtc::scoped_refptr<webrtc::I420Buffer> buffer =
		webrtc::I420Buffer::Create(target_width, target_height,
					   stride_y, stride_uv, stride_uv);

	libyuv::RotationMode rotation_mode = libyuv::kRotate0;

	const int conversionResult = libyuv::ConvertToI420(
		frame->data[0], size, buffer.get()->MutableDataY(),
		buffer.get()->StrideY(), buffer.get()->MutableDataU(),
		buffer.get()->StrideU(), buffer.get()->MutableDataV(),
		buffer.get()->StrideV(), 0, 0, outputWidth, outputHeight,
		target_width, target_height, rotation_mode,
		ConvertVideoType(videoType));

	// not using the result yet, silence compiler
	(void)conversionResult;

	const int64_t obs_timestamp_us =
		(int64_t)frame->timestamp / rtc::kNumNanosecsPerMicrosec;

	// Align timestamps from OBS capturer with rtc::TimeMicros timebase
	const int64_t aligned_timestamp_us =
		timestamp_aligner_.TranslateTimestamp(obs_timestamp_us,
						      rtc::TimeMicros());

	// Create a webrtc::VideoFrame to pass to the capturer
	webrtc::VideoFrame video_frame =
		webrtc::VideoFrame::Builder()
			.set_video_frame_buffer(buffer)
			.set_rotation(webrtc::kVideoRotation_0)
			.set_timestamp_us(aligned_timestamp_us)
			.set_id(++frame_id)
			.build();

	// Send frame to video capturer
	videoCapturer->OnFrameCaptured(video_frame);
}

// NOTE LUDO: #80 add getStats
void WebRTCStream::getStats()
{
	rtc::scoped_refptr<const webrtc::RTCStatsReport> report = NewGetStats();
	if (nullptr == report) {
		return;
	}
	stats_list = "";

	std::vector<const webrtc::RTCOutboundRTPStreamStats *> send_stream_stats =
		report->GetStatsOfType<webrtc::RTCOutboundRTPStreamStats>();
	for (const auto &stat : send_stream_stats) {
		if (stat->kind.ValueToString() == "audio") {
			audio_bytes_sent =
				std::stoll(stat->bytes_sent.ValueToJson());
		}
		if (stat->kind.ValueToString() == "video") {
			video_bytes_sent =
				std::stoll(stat->bytes_sent.ValueToJson());
			pli_received = std::stoi(stat->pli_count.ValueToJson());
		}
	}
	total_bytes_sent = audio_bytes_sent + video_bytes_sent;

	// RTCDataChannelStats
	std::vector<const webrtc::RTCDataChannelStats *> data_channel_stats =
		report->GetStatsOfType<webrtc::RTCDataChannelStats>();
	for (const auto &stat : data_channel_stats) {
		stats_list += "data_messages_sent:" +
			      stat->messages_sent.ValueToJson() + "\n";
		stats_list += "data_bytes_sent:" +
			      stat->bytes_sent.ValueToJson() + "\n";
		stats_list += "data_messages_received:" +
			      stat->messages_received.ValueToJson() + "\n";
		stats_list += "data_bytes_received:" +
			      stat->bytes_received.ValueToJson() + "\n";
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
			if (stat->audio_level.is_defined())
				stats_list += "track_audio_level:" +
					      stat->audio_level.ValueToJson() +
					      "\n";
			if (stat->total_audio_energy.is_defined())
				stats_list +=
					"track_total_audio_energy:" +
					stat->total_audio_energy.ValueToJson() +
					"\n";
			// stats_list += "track_echo_return_loss:"   + stat->echo_return_loss.ValueToJson() + "\n";
			// stats_list += "track_echo_return_loss_enhancement:" + stat->echo_return_loss_enhancement.ValueToJson() + "\n";
			// stats_list += "track_total_samples_received:" + stat->total_samples_received.ValueToJson() + "\n";
			if (stat->total_samples_duration.is_defined())
				stats_list += "track_total_samples_duration:" +
					      stat->total_samples_duration
						      .ValueToJson() +
					      "\n";
			// stats_list += "track_concealed_samples:" + stat->concealed_samples.ValueToJson() + "\n";
			// stats_list += "track_concealment_events:" + stat->concealment_events.ValueToJson() + "\n";
		}
		if (stat->kind.ValueToString() == "video") {
			// stats_list += "track_jitter_buffer_delay:"    + stat->jitter_buffer_delay.ValueToJson() + "\n";
			// stats_list += "track_jitter_buffer_emitted_count:" + stat->jitter_buffer_emitted_count.ValueToJson() + "\n";
			stats_list += "track_frame_width:" +
				      stat->frame_width.ValueToJson() + "\n";
			stats_list += "track_frame_height:" +
				      stat->frame_height.ValueToJson() + "\n";
			stats_list += "track_frames_sent:" +
				      stat->frames_sent.ValueToJson() + "\n";
			stats_list += "track_huge_frames_sent:" +
				      stat->huge_frames_sent.ValueToJson() +
				      "\n";

			// FPS = frames_sent / (time_now - start_time)
			std::chrono::system_clock::time_point time_now =
				std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds =
				time_now - previous_time;
			previous_time = time_now;
			uint32_t frames_sent =
				std::stol(stat->frames_sent.ValueToJson());
			stats_list += "track_fps:" +
				      std::to_string((frames_sent -
						      previous_frames_sent) /
						     elapsed_seconds.count()) +
				      "\n";
			previous_frames_sent = frames_sent;

			// stats_list += "track_frames_received:"        + stat->frames_received.ValueToJson() + "\n";
			// stats_list += "track_frames_decoded:"         + stat->frames_decoded.ValueToJson() + "\n";
			// stats_list += "track_frames_dropped:"         + stat->frames_dropped.ValueToJson() + "\n";
			// stats_list += "track_frames_corrupted:"       + stat->frames_corrupted.ValueToJson() + "\n";
			// stats_list += "track_partial_frames_lost:"    + stat->partial_frames_lost.ValueToJson() + "\n";
			// stats_list += "track_full_frames_lost:"       + stat->full_frames_lost.ValueToJson() + "\n";
		}
	}

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

	// RTCOutboundRTPStreamStats
	std::vector<const webrtc::RTCOutboundRTPStreamStats *>
		outbound_stream_stats = report->GetStatsOfType<
			webrtc::RTCOutboundRTPStreamStats>();
	for (const auto &stat : outbound_stream_stats) {
		if (stat->kind.ValueToString() == "audio") {
			stats_list += "outbound_audio_packets_sent:" +
				      stat->packets_sent.ValueToJson() + "\n";
			stats_list += "outbound_audio_bytes_sent:" +
				      stat->bytes_sent.ValueToJson() + "\n";
			// stats_list += "outbound_audio_target_bitrate:" + stat->target_bitrate.ValueToJson() + "\n";
			// stats_list += "outbound_audio_frames_encoded:" + stat->frames_encoded.ValueToJson() + "\n";
		}
		if (stat->kind.ValueToString() == "video") {
			stats_list += "outbound_video_packets_sent:" +
				      stat->packets_sent.ValueToJson() + "\n";
			stats_list += "outbound_video_bytes_sent:" +
				      stat->bytes_sent.ValueToJson() + "\n";
			// stats_list += "outbound_video_target_bitrate:" + stat->target_bitrate.ValueToJson() + "\n";
			// stats_list += "outbound_video_frames_encoded:" + stat->frames_encoded.ValueToJson() + "\n";
			stats_list += "outbound_video_fir_count:" +
				      stat->fir_count.ValueToJson() + "\n";
			stats_list += "outbound_video_pli_count:" +
				      stat->pli_count.ValueToJson() + "\n";
			stats_list += "outbound_video_nack_count:" +
				      stat->nack_count.ValueToJson() + "\n";
			// stats_list += "outbound_video_sli_count:"      + stat->sli_count.ValueToJson() + "\n";
			if (stat->qp_sum.is_defined())
				stats_list += "outbound_video_qp_sum:" +
					      stat->qp_sum.ValueToJson() + "\n";
		}
	}

	// RTCTransportStats
	std::vector<const webrtc::RTCTransportStats *> transport_stats =
		report->GetStatsOfType<webrtc::RTCTransportStats>();
	for (const auto &stat : transport_stats) {
		stats_list += "transport_bytes_sent:" +
			      stat->bytes_sent.ValueToJson() + "\n";
		stats_list += "transport_bytes_received:" +
			      stat->bytes_received.ValueToJson() + "\n";
	}
}

rtc::scoped_refptr<const webrtc::RTCStatsReport> WebRTCStream::NewGetStats()
{
	rtc::CritScope lock(&crit_);

	if (nullptr == pc) {
		return nullptr;
	}

	rtc::scoped_refptr<StatsCallback> stats_callback =
		new rtc::RefCountedObject<StatsCallback>();

	pc->GetStats(stats_callback);

	while (!stats_callback->called())
		std::this_thread::sleep_for(std::chrono::microseconds(1));

	return stats_callback->report();
}
