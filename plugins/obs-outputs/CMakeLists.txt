#cmake_minimum_required(VERSION 3.3)
project(obs-outputs)

# --- Find libwebrtc or fail early
find_package( WebRTC REQUIRED )
include( ${WEBRTC_USE_FILE} )
if( WIN32 )
  include_directories( "C:/Program Files/WebRTC/include" )
  link_directories( "C:/Program Files/WebRTC/lib" )
  add_definitions( -DWEBRTC_WIN -DNOMINMAX )
elseif( APPLE )
  add_definitions( -DWEBRTC_MAC -DWEBRTC_POSIX )
endif()

# --- platform specific libraries to link against
if(WIN32)
  set( obs-outputs_PLATFORM_DEPS
    ws2_32
    winmm
    Iphlpapi
  )
  if(MSVC)
    set( obs-outputs_PLATFORM_DEPS
      ${obs-outputs_PLATFORM_DEPS}
      w32-pthreads
    )
  endif()
endif()

# --- header and source files
set( obs-outputs_HEADERS
  obs-output-ver.h
  rtmp-helpers.h
  rtmp-stream.h
  net-if.h
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
  AudioDeviceModuleWrapper.cpp
  VideoCapturer.cpp
  WebRTCStream.cpp
  net-if.c
  )

# NOTE ALEX: since libwebrtc is compiled as a
# static link, we need to use this flag for the 
# dll to avoid linking issues with MSVC.
set(CMAKE_CXX_FLAGS_RELEASE "/MT")
set(CMAKE_CXX_FLAGS_DEBUG "/MTd")

# --- library target
add_library( obs-outputs MODULE
  ${obs-outputs_SOURCES}
  ${obs-outputs_HEADER}
)
target_link_libraries( obs-outputs
  ${obs-outputs_PLATFORM_DEPS}
  libobs
  websocketclient
  webrtc
  metrics_default
  field_trial_default
  )

install_obs_plugin_with_data(obs-outputs data)
