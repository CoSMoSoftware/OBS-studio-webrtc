#include "WebRTCStream.h"

#include <media-io/video-io.h>

#include <libyuv.h>
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include <modules/audio_processing/include/audio_processing.h>

#include <thread>
#include <chrono>
#include <memory>

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video/i420_buffer.h"
#include <rtc_base/platform_file.h>
#include <rtc_base/bitrate_allocation_strategy.h>
#include "rtc_base/checks.h"
#include "rtc_base/critical_section.h"

#include "pc/rtc_stats_collector.h"

#define warn( format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info( format, ...) blog(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG,   format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR,   format, ##__VA_ARGS__)

class CustomLogger : public rtc::LogSink
{
public:

  virtual void OnLogMessage(const std::string& message)
  {
    debug("webrtc: %s",message.c_str());
  };
};

CustomLogger logger;

WebRTCStream::WebRTCStream(obs_output_t * output)
{
  picId = 0;
  thumbnail = false;
  thumbnailDownrate = 1;
  thumbnailDownscale = 1;

  rtc::LogMessage::ConfigureLogging("info");
  rtc::LogMessage::AddLogToStream(&logger, rtc::LoggingSeverity::LS_VERBOSE);

  //Store output
  this->output = output;
  this->client = NULL;

  //Create audio device module
  adm = new rtc::RefCountedObject<AudioDeviceModuleWrapper>();

  //Network thread
  network = rtc::Thread::CreateWithSocketServer();
  network->SetName("network", nullptr);
  network->Start();

  //Worker thread
  worker = rtc::Thread::Create();
  worker->SetName("worker", nullptr);
  worker->Start();

  //Signaling thread
  signaling = rtc::Thread::Create();
  signaling->SetName("signaling", nullptr);
  signaling->Start();

  //Create peer connection factory with our audio wrapper module
  factory = webrtc::CreatePeerConnectionFactory(
    network.get(),
    worker.get(),
    signaling.get(),
    adm,
    webrtc::CreateBuiltinAudioEncoderFactory(),
    webrtc::CreateBuiltinAudioDecoderFactory(),
    webrtc::CreateBuiltinVideoEncoderFactory(),
    webrtc::CreateBuiltinVideoDecoderFactory(),
    nullptr,
    nullptr
  );

  //Create capture module with our custom one
  videoCapturer = new rtc::RefCountedObject<VideoCapturer>();

  //bitrate and dropped frame
  bitrate = 0;
  dropped_frame = 0;
  id = 0;
}

WebRTCStream::~WebRTCStream()
{
  stop();

  // Free factories first
  adm = NULL;
  pc = NULL;
  factory = NULL;
  videoCapturer = NULL;
  //thumbnailCapture = NULL;

  // Stop all thread
  if (!network->IsCurrent())   network->Stop();
  if (!worker->IsCurrent())    worker->Stop();
  if (!signaling->IsCurrent()) signaling->Stop();

  // Release
  network.release();
  worker.release();
  signaling.release();
}

bool WebRTCStream::start(Type type)
{
  this->type = type;
  //Get service
  obs_service_t *service = obs_output_get_service(output);

  if (!service)
    return false;

  // WebSocket URL sanity check
  if (!obs_service_get_url(service))
    return false;

  // the codec should be generic, and vp8 is the default if empty
  // possible values (should check): vp8, vp9, h264
  if (!obs_service_get_codec(service))
    codec = "vp8";
  else
    // should check the value is acceptable here
    // but since the input is supposedly a drop down menu, risk is low.
    codec = obs_service_get_codec(service);

  url = obs_service_get_url(service);
  milliId = "";
  milliToken = "";
  protocol = "";
  const char *tmpString = nullptr;
  const char *tmpToken  = nullptr;

  if (type == WebRTCStream::Millicast) {
    tmpString = obs_service_get_milli_id(    service );
    tmpToken  = obs_service_get_milli_token( service );
    milliId    = (NULL == tmpString ? "" : tmpString );
    milliToken = (NULL == tmpToken  ? "" : tmpToken  );
  } else {
    tmpString = obs_service_get_username(service);
    username = (NULL == tmpString ? "" : tmpString);
    tmpString = obs_service_get_password(service);
    password = (NULL == tmpString ? "" : tmpString);
    if (type == WebRTCStream::Wowza) {
      tmpString = obs_service_get_codec(service);
      codec = (NULL == tmpString ? "" : tmpString);
      tmpString = obs_service_get_protocol(service);
      protocol = (NULL == tmpString ? "" : tmpString);
    }
    try {
      room = obs_service_get_room(service);
    }
    catch (const std::invalid_argument& ia) {
    //  error("Invalid room name (must be a positive integer number)");
      return false;
    }
    catch (const std::out_of_range& oor) {
    //  error("Room name out of range (number too big)");
      return false;
    }
  }

  //Stop just in case
  stop();

  //Config
  webrtc::PeerConnectionInterface::RTCConfiguration config;
  webrtc::PeerConnectionInterface::IceServer server;
  server.uri = "stun:stun.l.google.com:19302";
  config.servers.push_back(server);

  //Create peer connection
  pc = factory->CreatePeerConnection(config, NULL, NULL, this);

  //Ensure it was created
  if (!pc.get()) {
    //Log
    error("Could not create PeerConnection");
    //Error
    return false;
  }

  //Create the media stream
  rtc::scoped_refptr<webrtc::MediaStreamInterface> stream = factory->CreateLocalMediaStream("obs");
  cricket::AudioOptions options;

  options.echo_cancellation.emplace(      false );
  options.auto_gain_control.emplace(      false );
  options.noise_suppression.emplace(      false );
  options.highpass_filter.emplace(        false );
  options.audio_jitter_buffer_max_packets.emplace(false);
  options.experimental_ns.emplace(        false );
  options.typing_detection.emplace(       false );
  options.residual_echo_detector.emplace( false );
  options.delay_agnostic_aec.emplace(     false );
  // options.aecm_generate_comfort_noise.emplace( false );
  // options.intelligibility_enhancer.emplace( false );
  // options.playout_sample_rate.emplace( false );
  // options.audio_network_adaptor.emplace( false );
  //

  //Add audio
  audio_track = factory->CreateAudioTrack("audio", factory->CreateAudioSource(options));
  //Add stream to track
  stream->AddTrack(audio_track);

  //Add video
  video_track = factory->CreateVideoTrack("video", videoCapturer);
  //Add stream to track
  stream->AddTrack(video_track);

  //Add the stream to the peer connection
  if (!pc->AddStream(stream)) {
    //Log
    error("Adding stream to PeerConnection failed");
    //Error
    return false;
  }

  //Create websocket client
  this->client = createWebsocketClient(type);
  //Check if it was created correctly
  if (!client) {
    //Error
    obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
    return false;
  }
  //Log them
  if (type == WebRTCStream::Millicast) {
    if (milliToken == "" || milliId == "") {
      error("Invalid token or publishing name");
      obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
      return false;
    }
    info("connecting to [url:%s,stream Id :%s,token :%s]", url.c_str(), milliId.c_str(), milliToken.c_str());
    if (!client->connect(url, room, username, milliToken , this)) {
      //Error
      obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
      return false;
    }
  } else if (type == WebRTCStream::Wowza) {
    info("connecting to [url: %s, protocol: %s, application name: %s, stream name: %s]",
        url.c_str(), protocol.empty() ? "Automatic" : protocol.c_str(), username.c_str(), password.c_str());
    if (!client->connect(url, room, username, password, this)) {
      error("Unable to connect to server");
      obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
      return false;
    }
  } else {
    info("connecting to [url:%s,room:%s,username:%s,password:%s]", url.c_str(), room.c_str(), username.c_str(), password.c_str());
    if (!client->connect(url, room, username, password, this)) {
      //Error
      obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
      return false;
    }
  }

  //OK
  return true;
}

void WebRTCStream::OnSuccess(webrtc::SessionDescriptionInterface * desc)
{
  std::string sdp;
  //Serialize sdp to string
  desc->ToString(&sdp);
  std::string sdpNotConst = sdp;
  //Got offer
  info("Got offer\r\n%s\n", sdp.c_str());

  int audio_bitrate = 0;

  obs_output_t *context = this->output;

  obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
  obs_data_t *vparams = obs_encoder_get_settings(vencoder);
  int video_bitrate = (int)obs_data_get_int(vparams, "bitrate");

  info("Video codec:   %s", codec.empty() ? "Automatic" : codec.c_str());
  info("Video bitrate: %d", video_bitrate);

  if (type == WebRTCStream::Wowza) {
    obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
    obs_data_t *aparams = obs_encoder_get_settings(aencoder);
    audio_bitrate = (int)obs_data_get_int(aparams, "bitrate");
    info("Audio bitrate: %d\n", audio_bitrate);
    std::vector<int> audio_payload_numbers;
    std::vector<int> video_payload_numbers;
    std::string audio_codec = "opus";
    std::string video_codec = this->codec;
    // If codec setting is Automatic
    if (codec.empty()) {
      audio_codec = "";
      video_codec = "";
    }
    // Force specific payload
    SDPModif::forcePayload(sdpNotConst, audio_payload_numbers,
        video_payload_numbers, audio_codec, video_codec, 0, "42e01f", 0);
    // Modify bitrate
    SDPModif::bitrateMaxMinSDP(sdpNotConst, video_bitrate, video_payload_numbers);
    // Enable stereo
    SDPModif::stereoSDP(sdpNotConst, audio_bitrate);
  }

  webrtc::SdpParseError error;
  // Create offer
  std::unique_ptr<webrtc::SessionDescriptionInterface> offer =
      webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdpNotConst, &error);
  info("OFFER:\n\n%s\n", sdpNotConst.c_str());
  //Set local description
  pc->SetLocalDescription(this, offer.release());
  //Send SDP
  info("WebRTCStream::OnSuccess: %s", codec.c_str());
  client->open(sdpNotConst, codec, milliId);
}

void WebRTCStream::OnFailure(const std::string & error)
{
  //Failed
  warn("Error [%s]", error.c_str());
  //Stop
  stop();
  //Disconnect
  obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::OnSuccess()
{
  //Got offer
  info("SDP set successfully\n");
}

void WebRTCStream::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
  //To string
  std::string str;
  candidate->ToString(&str);
  //Trickle
  client->trickle(candidate->sdp_mid(), candidate->sdp_mline_index(), str, false);
}

void WebRTCStream::onRemoteIceCandidate(const std::string &sdpData)
{
  if (sdpData.empty()) {
    info("ICE COMPLETE\n");
    pc->AddIceCandidate(nullptr);
  } else {
    std::string s = sdpData;
    s.erase(remove(s.begin(), s.end(), '\"'), s.end());
    if (protocol.empty() || SDPModif::filterIceCandidates(s, protocol)) {
      const std::string candidate = s;
      info("Remote %s\n", candidate.c_str());
      const std::string sdpMid = "";
      int sdpMLineIndex = 0;
      webrtc::SdpParseError error;
      const webrtc::IceCandidateInterface* newCandidate =
          webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex, candidate, &error);
      pc->AddIceCandidate(newCandidate);
    } else {
      info("Ignoring remote %s\n", s.c_str());
    }
  }
}

bool WebRTCStream::stop()
{
  //Stop PC
  if (!pc.get())
    //Exit
    return false;
  //Get pointer
  auto old = pc.release();
  //Close PC
  old->Close();
  //Check client
  if (client) {
    //Disconnect client
    client->disconnect(true);
    //Delete client
    delete(client);
    //NUll it
    client = NULL;
  }
  //Send end event
  obs_output_end_data_capture(output);
  return true;
}

void WebRTCStream::onConnected()
{
  //LOG
  info("onConnected");
}

void WebRTCStream::onLogged(int /*unused code*/)
{
  //LOG
  info("onLogged");
  //Create offer
  pc->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void WebRTCStream::onLoggedError(int code)
{
  //LOG
  error("onLoggedError [code:%d]",code);
  //Disconnect, this will call stop on main thread
  obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onOpened(const std::string &sdp)
{
  info("onOpened\r\n%s\n", sdp.c_str());
  std::string sdpNotConst = sdp;

  obs_output_t *context = this->output;

  obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
  obs_data_t *vparams = obs_encoder_get_settings(vencoder);
  int video_bitrate = (int)obs_data_get_int(vparams, "bitrate");

  obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, 0);
  obs_data_t *aparams = obs_encoder_get_settings(aencoder);
  int audio_bitrate = (int)obs_data_get_int(aparams, "bitrate");

  audio_bitrate = 0; // remove to enable

  info("Video codec:   %s", codec.c_str());
  info("Video bitrate: %d", video_bitrate);
  info("Audio bitrate: %d\n", audio_bitrate);

  if (type != WebRTCStream::Wowza) {
    // Modify video bitrate
    SDPModif::bitrateSDP(sdpNotConst, video_bitrate);
    // Enable stereo
    SDPModif::stereoSDP(sdpNotConst, audio_bitrate);
  }

  webrtc::SdpParseError error;
  webrtc::SessionDescriptionInterface* answer =
      webrtc::CreateSessionDescription(webrtc::SessionDescriptionInterface::kAnswer, sdpNotConst, &error);

  info("ANSWER\n\n%s\n", sdpNotConst.c_str());

  pc->SetRemoteDescription(this, answer);

  //Set audio data format
  audio_convert_info conversion;
  //Int 16bits, 48khz mono
  conversion.format = AUDIO_FORMAT_16BIT;
  conversion.samples_per_sec = 48000;
  conversion.speakers = SPEAKERS_STEREO;
  //Set it
  obs_output_set_audio_conversion(output, &conversion);

  //Start
  obs_output_begin_data_capture(output, 0);
}

void WebRTCStream::onOpenedError(int code)
{
  //LOG
  error("onOpenedError [code:%d]",code);
  //Disconnect, this will call stop on main thread
  obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onDisconnected()
{
  //LOG
  info("onDisconnected");
  //Disconnect, this will call stop on main thread
  obs_output_signal_stop(output, OBS_OUTPUT_ERROR);
}

void WebRTCStream::onVideoFrame(video_data *frame)
{
  if (!frame)
    return;
  if (!videoCapturer)
    return;

  //Calculate size
  int outputWidth = obs_output_get_width(output);
  int outputHeight = obs_output_get_height(output);
  auto videoType = webrtc::VideoType::kNV12;
  uint32_t size = outputWidth*outputHeight * 3 / 2;

  int stride_y = outputWidth;
  int stride_uv = (outputWidth + 1) / 2;
  int target_width = outputWidth;
  int target_height = outputHeight;

  // Create frame buffer
  rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(
    target_width, abs(target_height), stride_y, stride_uv, stride_uv);

  libyuv::RotationMode rotation_mode = libyuv::kRotate0;
  // Convert frame
  const int conversionResult = libyuv::ConvertToI420(
    frame->data[0], size,
    buffer.get()->MutableDataY(), buffer.get()->StrideY(),
    buffer.get()->MutableDataU(), buffer.get()->StrideU(),
    buffer.get()->MutableDataV(), buffer.get()->StrideV(), 0, 0, // No Cropping
    outputWidth, outputHeight, target_width, target_height,
    rotation_mode, ConvertVideoType(videoType)
  );
  // not using the result yet, silence compiler
  (void)conversionResult;

  const int64_t obs_timestamp_us = (int64_t)frame->timestamp / rtc::kNumNanosecsPerMicrosec;

  // Align timestamps from OBS capturer with rtc::TimeMicros timebase
  const int64_t aligned_timestamp_us = timestamp_aligner_.TranslateTimestamp(
    obs_timestamp_us, rtc::TimeMicros());

  // Create a webrtc::VideoFrame to pass to the capturer
  webrtc::VideoFrame video_frame = webrtc::VideoFrame::Builder()
    .set_video_frame_buffer(buffer)
    .set_rotation(webrtc::kVideoRotation_0)
    .set_timestamp_us(aligned_timestamp_us)
    .set_id(++id)
    .build();

  // Send frame to video capturer
  videoCapturer->OnFrameCaptured(video_frame);

  //Increase number of pictures
  picId++;
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
  if (!frame)
    return;
  //Push it to the device
  adm->onIncomingData(frame->data[0], frame->frames);
}

//bitrate and dropped_frame
// Disable for now
uint64_t WebRTCStream::getBitrate() {
  // rtc::scoped_refptr<webrtc::MockStatsObserver> observerAudio (new rtc::RefCountedObject<webrtc::MockStatsObserver> ());
  //
  //   pc->GetStats (observerVideo, video_track, webrtc::PeerConnectionInterface::kStatsOutputLevelStandard);
  // pc->GetStats (observerAudio, audio_track, webrtc::PeerConnectionInterface::kStatsOutputLevelStandard);
  //
  // std::this_thread::sleep_for(std::chrono::milliseconds(2));
  //
  //bitrate = observerVideo->BytesSent() + observerAudio->BytesSent();
  //
  return uint64_t(0);
}
