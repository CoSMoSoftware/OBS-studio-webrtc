// Copyright 2020 Evercast
// Copyright 2020 CoSMo - Dr Alex Gouaillard <contact@cosmosoftware.io>

#ifndef _OBS_WEBRTC_AUDIO_SOURCE_H_
#define _OBS_WEBRTC_AUDIO_SOURCE_H_

// lib obs include
#include <media-io/audio-io.h>

// webrtc includes
#include <api/scoped_refptr.h>
#include <api/notifier.h>
#include <api/peer_connection_interface.h>
#include <api/media_stream_interface.h>
#include <rtc_base/ref_counted_object.h>

// Glue class to use OBS audio capturer and proxy the audio data through to
// webrtc pipeline. Allows to fully control the audio capturing, and to reuse
// OBS settings, unlike the previous Audio Device Module Design.
class obsWebrtcAudioSource
	: public webrtc::Notifier<webrtc::AudioSourceInterface> {
public:
	static rtc::scoped_refptr<obsWebrtcAudioSource>
	Create(cricket::AudioOptions *options);

	// NOTE ALEX: FIXME
	SourceState state() const override { return kLive; }
	bool remote() const override { return false; }

	const cricket::AudioOptions options() const override
	{
		return options_;
	}

	~obsWebrtcAudioSource();

	void AddSink(webrtc::AudioTrackSinkInterface *sink) override;
	void RemoveSink(webrtc::AudioTrackSinkInterface *sink) override;
	void OnAudioData(audio_data *frame);

protected:
	audio_t *audio_;
	uint16_t pending_remainder;
	uint8_t *pending;

	// webrtc
	cricket::AudioOptions options_;
	webrtc::AudioTrackSinkInterface *sink_;
	obsWebrtcAudioSource();
	void Initialize(audio_t *audio, cricket::AudioOptions *options);
};

#endif
