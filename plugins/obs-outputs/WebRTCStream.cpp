#include "WebRTCStream.h"

#include <media-io/video-io.h>

#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/media/engine/webrtcvideocapturerfactory.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc/system_wrappers/include/field_trial.h"
/*

#include "webrtc/api/rtpreceiverinterface.h"
#include "webrtc/api/rtpsenderinterface.h"
#include "webrtc/api/videosourceproxy.h"
#include "webrtc/base/bind.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/event_tracer.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/logsinks.h"
#include "webrtc/base/messagequeue.h"
#include "webrtc/base/networkmonitor.h"
#include "webrtc/base/rtccertificategenerator.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/media/base/videocapturer.h"
#include "webrtc/media/engine/webrtcvideodecoderfactory.h"
#include "webrtc/media/engine/webrtcvideoencoderfactory.h"
#include "webrtc/system_wrappers/include/field_trial.h"
#include "webrtc/pc/webrtcsdp.h"
#include "webrtc/system_wrappers/include/field_trial_default.h"
#include "webrtc/system_wrappers/include/logcat_trace_context.h"
#include "webrtc/system_wrappers/include/trace.h"
#include "webrtc/voice_engine/include/voe_base.h"
using webrtc::VideoCaptureCapability;
using webrtc::VideoCaptureFactory;
using webrtc::VideoCaptureModule;
using cricket::WebRtcVideoDecoderFactory;
using cricket::WebRtcVideoEncoderFactory;
using rtc::Bind;
using rtc::Thread;
using rtc::ThreadManager;
using webrtc::AudioSourceInterface;
using webrtc::AudioTrackInterface;
using webrtc::AudioTrackVector;
using webrtc::CreateSessionDescriptionObserver;
using webrtc::DataBuffer;
using webrtc::DataChannelInit;
using webrtc::DataChannelInterface;
using webrtc::DataChannelObserver;
using webrtc::DtmfSenderInterface;
using webrtc::IceCandidateInterface;
using webrtc::LogcatTraceContext;
using webrtc::MediaConstraintsInterface;
using webrtc::MediaSourceInterface;
using webrtc::MediaStreamInterface;
using webrtc::MediaStreamTrackInterface;
using webrtc::PeerConnectionFactoryInterface;
using webrtc::PeerConnectionInterface;
using webrtc::PeerConnectionObserver;
using webrtc::RtpReceiverInterface;
using webrtc::RtpReceiverObserverInterface;
using webrtc::RtpSenderInterface;
using webrtc::SessionDescriptionInterface;
using webrtc::SetSessionDescriptionObserver;
using webrtc::StatsObserver;
using webrtc::StatsReport;
using webrtc::StatsReports;
using webrtc::VideoTrackSourceInterface;
using webrtc::VideoTrackInterface;
using webrtc::VideoTrackVector;
using webrtc::kVideoCodecVP8;
*/

#define warn(format, ...)  blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  blog(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG,   format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR,   format, ##__VA_ARGS__)

std::string webrtc::field_trial::FindFullName(const std::string& name)
{
	//No field trials
	return std::string();
}

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
	videoCaptureCapability.rawType = webrtc::RawVideoType::kVideoYV12;
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
	if (!network->IsCurrent())		network->Stop();
	if (!worker->IsCurrent())		worker->Stop();
	if (!signaling->IsCurrent())	signaling->Stop();
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

	//Stop just in case
	stop();

	//Config
	webrtc::PeerConnectionInterface::RTCConfiguration config;
	webrtc::FakeConstraints constraints;
	//webrtc::PeerConnectionInterface::IceServer server;
	//server.uri = GetPeerConnectionString();
	//config.servers.push_back(server);

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

	//Add audio
	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track = factory->CreateAudioTrack("audio", factory->CreateAudioSource(NULL));
	//Add stream to track
	stream->AddTrack(audio_track);

	//Create caprturer
	VideoCapturer* videoCapturer = new VideoCapturer(this);
	//Init it
	videoCapturer->Init(videoCapture);

	//Create video sorce
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

	//Get connection properties
	url = obs_service_get_url(service);
	key = obs_service_get_key(service);
	username = obs_service_get_username(service);
	password = obs_service_get_password(service);

	//Create websocket client
	this->client = createWebsocketClient();
	//Log them
	info("-connecting to [url:%s,key:%s,username:%s,password:%s]", url.c_str(), key.c_str(), username.c_str(), password.c_str());
	//Connect client
	if (!client->connect(url, key, username, password, this))
		//Error
		return false;

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
	//Calculate size
	videoCaptureCapability.width = obs_output_get_width(output);
	videoCaptureCapability.height = obs_output_get_height(output);
	videoCaptureCapability.rawType = webrtc::RawVideoType::kVideoNV12;
	//Calc size
	uint32_t size = videoCaptureCapability.width*videoCaptureCapability.height * 3 / 2; //obs_output_get_height(output) * frame->linesize[0];

	//Pass it
	videoCapture->IncomingFrame(frame->data[0], size, videoCaptureCapability);
}

void WebRTCStream::onAudioFrame(audio_data *frame)
{
	//Pash it to the device
	adm.onIncomingData(frame->data[0], frame->frames);
}
