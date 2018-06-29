#include "WebRTCStream.h"

#include <media-io/video-io.h>

#include <libyuv.h>
#include <rtc_base/platform_file.h>
#include <rtc_base/bitrateallocationstrategy.h>
#include <modules/audio_processing/include/audio_processing.h>

#include "api/test/fakeconstraints.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "media/engine/webrtcvideocapturerfactory.h"
#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/checks.h"
#include "rtc_base/criticalsection.h"

#include "pc/peerconnectionwrapper.h"
#include "pc/rtcstatscollector.h"

#include <iostream>
#include <fstream>
#include <future>

#define warn(format, ...)  blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  blog(LOG_INFO,    format, ##__VA_ARGS__)
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

    // rtc::LogMessage::ConfigureLogging("verbose debug timestamp");
    rtc::LogMessage::ConfigureLogging("info");
    // rtc::LogMessage::AddLogToStream(&logger, rtc::LoggingSeverity::LS_VERBOSE);
    //Store output
    this->output = output;
    this->client = NULL;
    //Block adm
    adm.AddRef();
    
    //Network thread
    network = rtc::Thread::CreateWithSocketServer();
    network->SetName("network", nullptr);
    network->Start();
    //Worker therad
    worker = rtc::Thread::Create();
    worker->SetName("worker", nullptr);
    worker->Start();
    //Worker therad
    signaling = rtc::Thread::Create();
    signaling->SetName("signaling", nullptr);
    signaling->Start();
    
    //Create peer connection factory with our audio wrapper module
    factory = webrtc::CreatePeerConnectionFactory(
                                                  network.get(),
                                                  worker.get(),
                                                  signaling.get(),
                                                  &adm,
                                                  webrtc::CreateBuiltinAudioEncoderFactory(),
                                                  webrtc::CreateBuiltinAudioDecoderFactory(),
                                                  nullptr,
                                                  nullptr
                                                  );
    
    //Create capture module with out custome one
    videoCapture = new VideoCapture();
    thumbnailCapture = new VideoCapture();

    //bitrate and dropped frame
    bitrate = 0;
    dropped_frame = 0;
}

WebRTCStream::~WebRTCStream()
{
    stop();
    //Free factories first
    pc = NULL;
    factory = NULL;
    videoCapture = NULL;
    thumbnailCapture = NULL;
    //Stop all thread
    if (!network->IsCurrent())   network->Stop();
    if (!worker->IsCurrent())    worker->Stop();
    if (!signaling->IsCurrent()) signaling->Stop();
    //Release
    network.release();
    worker.release();
    signaling.release();
    
}

bool WebRTCStream::start(Type type)
{

    //Get service
    obs_service_t *service = obs_output_get_service(output);
    
    if (!service)
        return false;
    // just in case
    if (!obs_service_get_url(service))
        return false;
    
    //Get connection properties
    url = obs_service_get_url(service);
    try {
        room = std::stoll(obs_service_get_room(service));
    }
    catch (const std::invalid_argument& ia) {
        error("Invalid room name (must be a positive integer number)");
        return false;
    }
    catch (const std::out_of_range& oor) {
        error("Room name out of range (number too big)");
        return false;
    }

    const char *tmpString = obs_service_get_username(service);
    username = (NULL == tmpString ? "" : tmpString);
    tmpString = obs_service_get_password(service);
    password = (NULL == tmpString ? "" : tmpString);

    //Stop just in case
    stop();
    
    //Config
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    webrtc::FakeConstraints constraints;
    webrtc::PeerConnectionInterface::IceServer server;
    server.uri = "stun:stun.l.google.com:19302";
    config.servers.push_back(server);
    
    //Create peer connection
    pc = factory->CreatePeerConnection(config, &constraints, NULL, NULL, this);
    
    //Ensure it was created
    if (!pc.get())
    {
        //Log
        error("Could not create PeerConnection");
        //Error
        return false;
    }

    //Create the media stream
    rtc::scoped_refptr<webrtc::MediaStreamInterface> stream = factory->CreateLocalMediaStream("obs");
    cricket::AudioOptions options;
   
    options.echo_cancellation.emplace(false);
    options.auto_gain_control.emplace(false);
    options.noise_suppression.emplace(false);
    options.highpass_filter.emplace(false);
    options.audio_jitter_buffer_max_packets.emplace(false);
    options.experimental_ns.emplace(false);
    options.aecm_generate_comfort_noise.emplace(false);
    options.typing_detection.emplace(false);
    options.residual_echo_detector.emplace(false);
    options.delay_agnostic_aec.emplace(false);
/*    
    options.intelligibility_enhancer.emplace(false);
    options.playout_sample_rate.emplace(false);
    options.audio_network_adaptor.emplace(false);
*/
    //Add audio
    audio_track = factory->CreateAudioTrack("audio", factory->CreateAudioSource(options));
    //Add stream to track
    stream->AddTrack(audio_track);
    
    //Create caprturer
    VideoCapturer* videoCapturer = new VideoCapturer(this);
    //Init it
    videoCapturer->Init(videoCapture);

    //Create video source
    rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource = factory->CreateVideoSource(videoCapturer, NULL);
    
    //Add video
    video_track = factory->CreateVideoTrack("video", videoSource);
    //Add stream to track
    stream->AddTrack(video_track);

    //If doing thumnails
    if (thumbnail)
    {
      //Create caprturer
      VideoCapturer* thumbnailCapturer = new VideoCapturer(this);
      //Init it
      thumbnailCapturer->Init(thumbnailCapture);

      //Create thumbnail source
      rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> thumbnailSource = factory->CreateVideoSource(thumbnailCapturer, NULL);

      //Add thumbnail
      rtc::scoped_refptr<webrtc::VideoTrackInterface> thumbnail_track = factory->CreateVideoTrack("thumbnail", thumbnailSource);
      //Add stream to track
      stream->AddTrack(thumbnail_track);
    }
    
    //Add the stream to the peer connection
    if (!pc->AddStream(stream))
    {
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
    info("-connecting to [url:%s,room:%ld,username:%s,password:%s]", url.c_str(), room, username.c_str(), password.c_str());
    //Connect client
    if (!client->connect(url, room, username, password, this)){
        //Error
        obs_output_signal_stop(output, OBS_OUTPUT_CONNECT_FAILED);
        return false;
    }
    //OK
    return true;
}

void WebRTCStream::OnSuccess(webrtc::SessionDescriptionInterface * desc)
{
    std::string sdp;
    //Serialize sdp to string
    desc->ToString(&sdp);
    //Got offer
    info("Got offer\r\n%s", sdp.c_str());
    //Set local description
    pc->SetLocalDescription(this, desc);
    //Send SDP
    client->open(sdp, codec);
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
    info("SDP set sucessfully");
}

void WebRTCStream::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    //To string
    std::string str;
    candidate->ToString(&str);
    //Trickle
    client->trickle(candidate->sdp_mid(), candidate->sdp_mline_index(), str, false);
};

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
    if (client)
    {
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

void WebRTCStream::onLogged(int code)
{
    //LOG
    info("onLogged");
    //Create offer
    pc->CreateOffer(this, NULL);
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
    info("onOpened\r\n%s", sdp.c_str());
    std::string sdpNotConst = sdp;
    // Enable stereo
    Stereo::stereoSDP(sdpNotConst);
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface* answer =
    webrtc::CreateSessionDescription(webrtc::SessionDescriptionInterface::kAnswer, sdpNotConst, &error);
    
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
    if (!videoCapture)
        return;
    
    //Calculate size
    videoCaptureCapability.width = obs_output_get_width(output);
    videoCaptureCapability.height = obs_output_get_height(output);
    videoCaptureCapability.videoType = webrtc::VideoType::kNV12;    
    //Calc size
    uint32_t size = videoCaptureCapability.width*videoCaptureCapability.height * 3 / 2;
    //Pass it
    videoCapture->IncomingFrame(frame->data[0], size, videoCaptureCapability);

    //If we are doing thumbnails and we are not skiping this frame
    if (thumbnail && thumbnailCapture && ((picId % thumbnailDownrate)==0))
    {
	
       //Calculate size
       thumbnailCaptureCapability.width = obs_output_get_width(output) / thumbnailDownscale;
       thumbnailCaptureCapability.height = obs_output_get_height(output) / thumbnailDownscale;
       thumbnailCaptureCapability.videoType = webrtc::VideoType::kI420;
       //Calc size of the downscaled version
       uint32_t donwscaledSize = thumbnailCaptureCapability.width*thumbnailCaptureCapability.height*3/2;
       //Create new image for the I420 conversion and the downscaling
       uint8_t* downscaled = (uint8_t*)malloc(size+donwscaledSize);

       //Get planar pointers
       uint8_t *dest_y = (uint8_t *)downscaled;
       uint8_t *dest_u = (uint8_t *)dest_y + videoCaptureCapability.width*videoCaptureCapability.height;
       uint8_t *dest_v = (uint8_t *)dest_y + videoCaptureCapability.width*videoCaptureCapability.height * 5 / 4;
       uint8_t *resc_y = (uint8_t *)downscaled + size;
       uint8_t *resc_u = (uint8_t *)resc_y + thumbnailCaptureCapability.width*thumbnailCaptureCapability.height;
       uint8_t *resc_v = (uint8_t *)resc_y + thumbnailCaptureCapability.width*thumbnailCaptureCapability.height * 5 / 4;

       //Convert first to I420
       libyuv::NV12ToI420(
	       (uint8_t *)frame->data[0], frame->linesize[0],
	       (uint8_t *)frame->data[1], frame->linesize[1],
	       dest_y, videoCaptureCapability.width,
	       dest_u, videoCaptureCapability.width/2,
	       dest_v, videoCaptureCapability.width/2,
	       videoCaptureCapability.width, videoCaptureCapability.height);

       //Rescale
       libyuv::I420Scale(
	       dest_y, videoCaptureCapability.width,
	       dest_u, videoCaptureCapability.width / 2,
	       dest_v, videoCaptureCapability.width / 2,
	       videoCaptureCapability.width, videoCaptureCapability.height,
	       resc_y, thumbnailCaptureCapability.width,
	       resc_u, thumbnailCaptureCapability.width / 2,
	       resc_v, thumbnailCaptureCapability.width / 2,
	       thumbnailCaptureCapability.width, thumbnailCaptureCapability.height,
	       libyuv::kFilterNone
       );
       //Pass it
       thumbnailCapture->IncomingFrame(resc_y, donwscaledSize, thumbnailCaptureCapability);
       //Free downscaled version
       free(downscaled);
    }

    //Increase number of pictures
    picId++;
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
    if (!frame)
        return;
    //Push it to the device
    adm.onIncomingData(frame->data[0], frame->frames);
}

//bitrate and dropped_frame
int WebRTCStream::findValueJson(std::string json, std::string id) {
    int pos = json.find(id);
    int value = std::stoi(json.substr(pos + 1));
    return value;
}

uint64_t WebRTCStream::getBitrate() {
    rtc::scoped_refptr<webrtc::MockStatsObserver> observerVideo (new rtc::RefCountedObject<webrtc::MockStatsObserver> ());
	rtc::scoped_refptr<webrtc::MockStatsObserver> observerAudio (new rtc::RefCountedObject<webrtc::MockStatsObserver> ());

    pc->GetStats (observerVideo, video_track, webrtc::PeerConnectionInterface::kStatsOutputLevelStandard);
	pc->GetStats (observerAudio, audio_track, webrtc::PeerConnectionInterface::kStatsOutputLevelStandard);

	Sleep(5);
    
	bitrate = observerVideo->BytesSent() + observerAudio->BytesSent();
    
	return bitrate;
}

int WebRTCStream::getDroppedFrame() {
    //dropped_frame = findValueJson(statsCollectorCallback.getReport(), "droppedFrame");
    return dropped_frame;
}
