#cmake_minimum_required(VERSION 3.3)
project(obs-outputs)

# --- Find libwebrtc or fail early
find_package(libwebrtc REQUIRED)



# --- platform specific libraries to link against
if(WIN32)
  set( obs-outputs_PLATFORM_DEPS
    ${WEBRTC_LIBRARIES}
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
