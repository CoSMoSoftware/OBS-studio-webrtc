#cmake_minimum_required(VERSION 3.3)
project(obs-outputs)

# --- Find libwebrtc or fail early
find_package( WebRTC REQUIRED )
include( ${WEBRTC_USE_FILE} )
if( WIN32 )
  include_directories( "C:/Program Files/WebRTC/include" )
  link_directories( "C:/Program Files/WebRTC/lib" )
endif()

# --- platform specific libraries to link against
if(WIN32)
  set( obs-outputs_PLATFORM_DEPS
    ws2_32
    winmm
    Iphlpapi
  )
endif()

if(MSVC)
  set( obs-outputs_PLATFORM_DEPS
    ${obs-outputs_PLATFORM_DEPS}
    w32-pthreads
  )
endif()

# --- header and source files
set( obs-outputs_HEADERS
  obs-output-ver.h
  rtmp-helpers.h
  rtmp-stream.h
  net-if.h
  flv-mux.h
  flv-output.h
  AudioDeviceModuleWrapper.h
  VideoCapture.h
  VideoCapturer.h
  WebRTCStream.h
  WebsocketClient.h
  )
set( obs-outputs_SOURCES
  obs-outputs.c
  rtmp-stream.cpp
  rtmp-windows.c
  flv-output.c
  flv-mux.c
  AudioDeviceModuleWrapper.cpp
  VideoCapturer.cpp
  WebRTCStream.cpp
  net-if.c
  )

# --- library target
add_library( obs-outputs MODULE
  ${obs-outputs_SOURCES}
  ${obs-outputs_HEADER}
)
target_link_libraries( obs-outputs
  ${obs-outputs_PLATFORM_DEPS}
  libobs
  webrtc
  metrics_default
  field_trial_default
)

install_obs_plugin_with_data(obs-outputs data)
