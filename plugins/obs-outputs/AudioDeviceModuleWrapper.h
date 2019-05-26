#ifndef _AUDIO_DEVICE_MODULE_WRAPPER_H_
#define _AUDIO_DEVICE_MODULE_WRAPPER_H_

#include <stdio.h>

#include "modules/audio_device/audio_device_generic.h"
#include "rtc_base/refcountedobject.h"
#include "rtc_base/criticalsection.h"
#include "rtc_base/checks.h"

using webrtc::AudioDeviceBuffer;
using webrtc::AudioDeviceGeneric;
using webrtc::AudioDeviceModule;
using webrtc::AudioTransport;
using webrtc::kAdmMaxDeviceNameSize;
using webrtc::kAdmMaxGuidSize;
using webrtc::kAdmMaxFileNameSize;

class AudioDeviceModuleWrapper 
  : public rtc::RefCountedObject<AudioDeviceModule>
{
public:
  AudioDeviceModuleWrapper();
  ~AudioDeviceModuleWrapper();

  virtual int64_t TimeUntilNextProcess() { return 1000; }
  virtual void Process() {}

  // Retrieve the currently utilized audio layer
  virtual int32_t ActiveAudioLayer(AudioLayer* audioLayer) const 
  { 
    *audioLayer = AudioLayer::kDummyAudio;
    return 0;
  }

  // Error handling
  virtual ErrorCode LastError() const { return (ErrorCode)0;  }

  // Full-duplex transportation of PCM audio
  virtual int32_t RegisterAudioCallback(AudioTransport* audioTransport)
  {
    this->audioTransport = audioTransport;
    return 0;
  }

  // Main initialization and termination
  virtual int32_t Init();
  virtual int32_t Terminate();
  virtual bool Initialized() const;

  // Device enumeration
  virtual int16_t PlayoutDevices()   {  return 0; }
  virtual int16_t RecordingDevices() { return 1; }
  virtual int32_t PlayoutDeviceName(  uint16_t index, char name[kAdmMaxDeviceNameSize], char guid[kAdmMaxGuidSize])   { return 0; }
  virtual int32_t RecordingDeviceName(uint16_t index, char name[kAdmMaxDeviceNameSize], char guid[kAdmMaxGuidSize]) 
  {
    sprintf(name, "rtmp_stream");
    sprintf(name, "obs");
    return 0;
  }

  // Device selection
  virtual int32_t SetPlayoutDevice(    uint16_t ) { return 0; }
  virtual int32_t SetRecordingDevice(  uint16_t ) { return 0; }
  virtual int32_t SetPlayoutDevice(   WindowsDeviceType ) { return 0; }
  virtual int32_t SetRecordingDevice( WindowsDeviceType ) { return 0; }

  // Audio transport initialization
  virtual int32_t PlayoutIsAvailable( bool* ) { return 0; }
  virtual int32_t InitPlayout() { return 0; }
  virtual bool PlayoutIsInitialized() const { return 0; }
  virtual int32_t RecordingIsAvailable( bool* ) { return 0; }
  virtual int32_t InitRecording() { return 0; }
  virtual bool RecordingIsInitialized() const { return 0;  }

  // Audio transport control
  virtual int32_t StartPlayout()   { return 0; }
  virtual int32_t StopPlayout()    { return 0; }
  virtual bool Playing()     const { return 0; }
  virtual int32_t StartRecording() { return 0; }
  virtual int32_t StopRecording()  { return 0; }
  virtual bool Recording()   const { return true; }

  // Microphone Automatic Gain Control (AGC)
  virtual int32_t SetAGC( bool ) { return 0; }
  virtual bool AGC() const { return false; }

  // Volume control based on the Windows Wave API (Windows only)
  virtual int32_t SetWaveOutVolume( uint16_t,  uint16_t  )       { return 0; }
  virtual int32_t WaveOutVolume(    uint16_t*, uint16_t* ) const { return 0; }

  // Audio mixer initialization
  virtual int32_t InitSpeaker()                { return 0; }
  virtual bool SpeakerIsInitialized()    const { return 0; }
  virtual int32_t InitMicrophone()             { return 0; }
  virtual bool MicrophoneIsInitialized() const { return true; }

  // Speaker volume controls
  virtual int32_t SpeakerVolumeIsAvailable( bool*     )       { return 0; }
  virtual int32_t SetSpeakerVolume(         uint32_t  )       { return 0; }
  virtual int32_t SpeakerVolume(            uint32_t* ) const { return 0; }
  virtual int32_t MaxSpeakerVolume(         uint32_t* ) const { return 0; }
  virtual int32_t MinSpeakerVolume(         uint32_t* ) const { return 0; }
  virtual int32_t SpeakerVolumeStepSize(    uint16_t* ) const { return 0; }

  // Microphone volume controls
  virtual int32_t MicrophoneVolumeIsAvailable( bool*     )       { return 0; }
  virtual int32_t SetMicrophoneVolume(         uint32_t  )       { return 0; }
  virtual int32_t MicrophoneVolume(            uint32_t* ) const { return 0; }
  virtual int32_t MaxMicrophoneVolume(         uint32_t* ) const { return 0; }
  virtual int32_t MinMicrophoneVolume(         uint32_t* ) const { return 0; }
  virtual int32_t MicrophoneVolumeStepSize(    uint16_t* ) const { return 0; }

  // Speaker mute control
  virtual int32_t SpeakerMuteIsAvailable( bool* )       { return 0; }
  virtual int32_t SetSpeakerMute(         bool  )       { return 0; }
  virtual int32_t SpeakerMute(            bool* ) const { return 0; }

  // Microphone mute control
  virtual int32_t MicrophoneMuteIsAvailable( bool* )       { return 0; }
  virtual int32_t SetMicrophoneMute(         bool  )       { return 0; }
  virtual int32_t MicrophoneMute(            bool* ) const { return 0; }

  // Microphone boost control
  virtual int32_t MicrophoneBoostIsAvailable( bool* )       { return 0; }
  virtual int32_t SetMicrophoneBoost(         bool  )       { return 0; }
  virtual int32_t MicrophoneBoost(            bool* ) const { return 0; }

  // Stereo support
  virtual int32_t StereoPlayoutIsAvailable(   bool* ) const { return 0; }
  virtual int32_t SetStereoPlayout(           bool  )       { return 0; }
  virtual int32_t StereoPlayout(              bool* ) const { return 0; }
  virtual int32_t StereoRecordingIsAvailable( bool* ) const { return 0; }
  virtual int32_t SetStereoRecording(         bool  )       { return 0; }
  virtual int32_t StereoRecording(            bool* ) const { return 0; }
  virtual int32_t SetRecordingChannel(        const ChannelType  )       { return 0; }
  virtual int32_t RecordingChannel(                 ChannelType* ) const { return 0; }

  // Delay information and control
  virtual int32_t PlayoutDelay(   uint16_t* ) const { return 0; }
  virtual int32_t RecordingDelay( uint16_t* ) const { return 0; }

  // CPU load
  virtual int32_t CPULoad( uint16_t* ) const { return 0; }

  // Recording of raw PCM data
  virtual int32_t StartRawOutputFileRecording( const char pcmFileNameUTF8[kAdmMaxFileNameSize]) { return 0; }
  virtual int32_t StartRawInputFileRecording(  const char pcmFileNameUTF8[kAdmMaxFileNameSize]) { return 0; }
  virtual int32_t StopRawOutputFileRecording() { return 0; }
  virtual int32_t StopRawInputFileRecording()  { return 0; }

  // Native sample rate controls (samples/sec)
  virtual int32_t SetRecordingSampleRate( const uint32_t  )       { return 0; }
  virtual int32_t RecordingSampleRate(          uint32_t* ) const { return 0; }
  virtual int32_t SetPlayoutSampleRate(   const uint32_t  )       { return 0; }
  virtual int32_t PlayoutSampleRate(            uint32_t* ) const { return 0; }

  // Mobile device specific functions
  virtual int32_t ResetAudioDevice()                  { return 0; }
  virtual int32_t SetLoudspeakerStatus( bool  )       { return 0; }
  virtual int32_t GetLoudspeakerStatus( bool* ) const { return 0; }

  // Only supported on Android.
  virtual bool BuiltInAECIsAvailable() const { return false; }
  virtual bool BuiltInAGCIsAvailable() const { return false; }
  virtual bool BuiltInNSIsAvailable()  const { return false; }

  // Enables the built-in audio effects. Only supported on Android.
  virtual int32_t EnableBuiltInAEC( bool ) { return 0; }
  virtual int32_t EnableBuiltInAGC( bool ) { return 0; }
  virtual int32_t EnableBuiltInNS(  bool ) { return 0; }

  void onIncomingData(uint8_t* data, size_t samples_per_channel);

public:
  bool                 _initialized;
  rtc::CriticalSection _critSect;
  AudioTransport*      audioTransport;
  uint8_t              pending[640 * 2 * 2];
  size_t               pendingLength;
};

#endif
