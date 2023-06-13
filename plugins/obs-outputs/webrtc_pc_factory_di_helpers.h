#ifndef _WEBRTC_PC_FACTORY_DI_HELPERS_H_
#define _WEBRTC_PC_FACTORY_DI_HELPERS_H_

/// WebRTC Dependency Injection Helpers for the PeerConnection Factory
/// this allows us to inject custom implementations of peer connection factory modules,
/// like network controllers, without having to patch libwebrtc.

#include "api/create_peerconnection_factory.h"
#include "api/peer_connection_interface.h"

#include <memory>
#include <utility>

#include "api/call/call_factory_interface.h"
#include "api/peer_connection_interface.h"
#include "api/rtc_event_log/rtc_event_log_factory.h"
#include "api/scoped_refptr.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/transport/field_trial_based_config.h"
#include "media/base/media_engine.h"
#include "media/engine/webrtc_media_engine.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/thread.h"

namespace webrtc
{

  inline PeerConnectionFactoryDependencies CreateDefaultPeerConnectionFactoryDependencies(
      rtc::Thread *network_thread,
      rtc::Thread *worker_thread,
      rtc::Thread *signaling_thread,
      rtc::scoped_refptr<AudioDeviceModule> default_adm,
      rtc::scoped_refptr<AudioEncoderFactory> audio_encoder_factory,
      rtc::scoped_refptr<AudioDecoderFactory> audio_decoder_factory,
      std::unique_ptr<VideoEncoderFactory> video_encoder_factory,
      std::unique_ptr<VideoDecoderFactory> video_decoder_factory,
      rtc::scoped_refptr<AudioMixer> audio_mixer,
      rtc::scoped_refptr<AudioProcessing> audio_processing,
      AudioFrameProcessor *audio_frame_processor = nullptr,
      std::unique_ptr<FieldTrialsView> field_trials = nullptr)
  {
    if (!field_trials)
    {
      field_trials = std::make_unique<webrtc::FieldTrialBasedConfig>();
    }

    PeerConnectionFactoryDependencies dependencies;
    dependencies.network_thread = network_thread;
    dependencies.worker_thread = worker_thread;
    dependencies.signaling_thread = signaling_thread;
    dependencies.task_queue_factory =
        CreateDefaultTaskQueueFactory(field_trials.get());
    dependencies.call_factory = CreateCallFactory();
    dependencies.event_log_factory = std::make_unique<RtcEventLogFactory>(
        dependencies.task_queue_factory.get());
    dependencies.trials = std::move(field_trials);

    if (network_thread)
    {
      // TODO(bugs.webrtc.org/13145): Add an rtc::SocketFactory* argument.
      dependencies.socket_factory = network_thread->socketserver();
    }
    cricket::MediaEngineDependencies media_dependencies;
    media_dependencies.task_queue_factory = dependencies.task_queue_factory.get();
    media_dependencies.adm = std::move(default_adm);
    media_dependencies.audio_encoder_factory = std::move(audio_encoder_factory);
    media_dependencies.audio_decoder_factory = std::move(audio_decoder_factory);
    media_dependencies.audio_frame_processor = audio_frame_processor;
    if (audio_processing)
    {
      media_dependencies.audio_processing = std::move(audio_processing);
    }
    else
    {
      media_dependencies.audio_processing = AudioProcessingBuilder().Create();
    }
    media_dependencies.audio_mixer = std::move(audio_mixer);
    media_dependencies.video_encoder_factory = std::move(video_encoder_factory);
    media_dependencies.video_decoder_factory = std::move(video_decoder_factory);
    media_dependencies.trials = dependencies.trials.get();
    dependencies.media_engine =
        cricket::CreateMediaEngine(std::move(media_dependencies));

    return dependencies;
  }
}

#endif // _WEBRTC_PC_FACTORY_DI_HELPERS_H_
