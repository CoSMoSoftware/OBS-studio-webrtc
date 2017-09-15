#include "AudioDeviceModuleWrapper.h"
#include "webrtc/rtc_base/timeutils.h"
#include "obs.h"
#include "media-io/audio-io.h"

AudioDeviceModuleWrapper::AudioDeviceModuleWrapper() :
	_initialized(false)
{
	pendingLength = 0;
}


AudioDeviceModuleWrapper::~AudioDeviceModuleWrapper()
{
}


// ----------------------------------------------------------------------------
//  Init
// ----------------------------------------------------------------------------

int32_t AudioDeviceModuleWrapper::Init()
{
	if (_initialized)
		return 0;
	
	_initialized = true;
	return 0;
}

// ----------------------------------------------------------------------------
//  Terminate
// ----------------------------------------------------------------------------

int32_t AudioDeviceModuleWrapper::Terminate()
{
	if (!_initialized)
		return 0;

	_initialized = false;
	return 0;
}

// ----------------------------------------------------------------------------
//  Initialized
// ----------------------------------------------------------------------------

bool AudioDeviceModuleWrapper::Initialized() const
{
	return (_initialized);
}


void AudioDeviceModuleWrapper::onIncomingData(uint8_t* data, size_t samples_per_channel)
{
	_critSect.Enter();
	if (!audioTransport)
		return;
	_critSect.Leave();

	//Get audio
	audio_t *audio = obs_get_audio();
	//This info is set on the stream before starting capture
	size_t channels = 2;
	size_t sample_rate = 48000;
	size_t sample_size = 2;
	//Get chunk for 10ms
	size_t chunk = (sample_rate / 100);

	size_t i = 0;
	uint32_t level;

	//Check if we had pending
	if (pendingLength)
	{
		//Copy the missing ones
		i = chunk - pendingLength;
		//Copy 
		memcpy(pending + pendingLength*sample_size*channels, data, i*sample_size*channels);

		//Add sent
		audioTransport->RecordedDataIsAvailable(pending, chunk, sample_size, channels, sample_rate, 0, 0, 0, 0, level);

		//No pending
		pendingLength = 0;
	}

	//Send all full chunks possible
	while ( i + chunk < samples_per_channel)
	{
		//Send them
		audioTransport->RecordedDataIsAvailable(data + i*sample_size*channels, chunk, sample_size, channels, sample_rate, 0, 0, 0, 0, level);
		//Inc sent
		i += chunk;
	}

	//If there are mising ones
	if (i != samples_per_channel)
	{
		//Calcualte pending
		pendingLength = samples_per_channel - i;
		//Copy to pending buffer
		memcpy(pending, data + i*sample_size*channels, pendingLength*sample_size*channels);
	}
}
