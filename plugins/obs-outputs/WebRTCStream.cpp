#include "WebRTCStream.h"

#include <media-io/video-io.h>

#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/media/engine/webrtcvideocapturerfactory.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc/rtc_base/checks.h"
#include "webrtc/rtc_base/criticalsection.h"

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
    
    //rtc::LogMessage::ConfigureLogging("verbose debug timestamp");
    rtc::LogMessage::ConfigureLogging("info");
    //rtc::LogMessage::AddLogToStream(&logger, rtc::LoggingSeverity::LS_VERBOSE);
    
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
                                                  nullptr,
                                                  nullptr
                                                  );
    
    //Create capture module with out custome one
    videoCapture = new VideoCapture();
    
    //Always YUV2
    videoCaptureCapability.videoType = webrtc::VideoType::kYV12;    //Calc size
    /*
    webrtc::PeerConnectionFactoryInterface::Options options;
    options.disable_network_monitor = false;
    */
}

WebRTCStream::~WebRTCStream()
{
    stop();
    //Free factories first
    pc = NULL;
    factory = NULL;
    videoCapture = NULL;
    //Stop all thread
    if (!network->IsCurrent())    network->Stop();
    if (!worker->IsCurrent())    worker->Stop();
    if (!signaling->IsCurrent())  signaling->Stop();
    //Release
    network.release();
    worker.release();
    signaling.release();
    
}

bool WebRTCStream::start()
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
    key = obs_service_get_key(service);
    username = obs_service_get_username(service);
    password = obs_service_get_password(service);
    
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
    rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track = factory->CreateAudioTrack("audio", factory->CreateAudioSource(options));
    //Add stream to track
    stream->AddTrack(audio_track);
    
    //Create caprturer
    VideoCapturer* videoCapturer = new VideoCapturer(this);
    //Init it
    videoCapturer->Init(videoCapture);
    
    //Create video source
    rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource = factory->CreateVideoSource(videoCapturer, NULL);
    
    //Add video
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track = factory->CreateVideoTrack("video", videoSource);
    //Add stream to track
    stream->AddTrack(video_track);
    
    //Add the stream to the peer connection
    if (!pc->AddStream(stream))
    {
        //Log
        error("Adding stream to PeerConnection failed");
        //Error
        return false;
    }
    
    //Create websocket client
    this->client = createWebsocketClient();
    //Log them
    info("-connecting to [url:%s,key:%s,username:%s,password:%s]", url.c_str(), key.c_str(), username.c_str(), password.c_str());
    //Connect client
    if (!client->connect(url, key, username, password, this)){
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
    client->open(sdp);
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
    
    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface* answer =
    webrtc::CreateSessionDescription(webrtc::SessionDescriptionInterface::kAnswer,sdp,&error);
    
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
    uint32_t size = videoCaptureCapability.width*videoCaptureCapability.height * 3 / 2; //obs_output_get_height(output) * frame->linesize[0];
    //Pass it
    videoCapture->IncomingFrame(frame->data[0], size, videoCaptureCapability);
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
    if (!frame)
        return;
    //Pash it to the device
    adm.onIncomingData(frame->data[0], frame->frames);
}
