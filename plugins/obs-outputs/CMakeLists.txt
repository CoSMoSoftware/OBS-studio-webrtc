#cmake_minimum_required(VERSION 3.3)
project(obs-outputs)

# --- Find libwebrtc or fail early
find_package(LibWebRTC REQUIRED)
include(${LIBWEBRTC_USE_FILE})

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
    webrtc
    metrics_default
    field_trial_default
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
elseif( APPLE )
  find_library( FOUNDATION_LIBRARY   Foundation   )
  find_library( AVFOUNDATION_LIBRARY AVFoundation )
  find_library( COREMEDIA_LIBRARY    CoreMedia    )
  find_library( COREGRAFX_LIBRARY    CoreGraphics )
  find_library( COREVIDEO_LIBRARY    CoreVideo    )
  find_library( COREAUDIO_LIBRARY    CoreAudio    )
  find_library( AUDIOTOOLBOX_LIBRARY AudioToolbox )
  set( obs-outputs_PLATFORM_DEPS
    ${LIBWEBRTC_LIBRARIES}
    ${FOUNDATION_LIBRARY}
    ${AVFOUNDATION_LIBRARY}
    ${COREMEDIA_LIBRARY}
    ${COREGRAFX_LIBRARY}
    ${COREVIDEO_LIBRARY}
    ${COREAUDIO_LIBRARY}
    ${AUDIOTOOLBOX_LIBRARY}
  )
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

if ( WIN32 )
  set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

# --- library target
add_library( obs-outputs MODULE
  ${obs-outputs_SOURCES}
  ${obs-outputs_HEADER}
)
target_link_libraries( obs-outputs
  ${obs-outputs_PLATFORM_DEPS}
  libobs
  websocketclient
  )

install_obs_plugin_with_data(obs-outputs data)
