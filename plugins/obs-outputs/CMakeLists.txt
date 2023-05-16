#-------------------------------------------------------------------------------
# - OBS outputs
#
# * RTMP output
# * WEBRTC output (WHIP signalling)
# * WEBRTC output (Millicast signalling)
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

find_package(MbedTLS REQUIRED)
find_package(ZLIB REQUIRED)

#-------------------------------------------------------------------------------
# WebRTC
#-------------------------------------------------------------------------------

if(WIN32)
	set(WEBRTC_USE_FILE_INCLUDED 0)
endif()

# --- libwebrtc

message( STATUS "Looking for libwebrtc" )

# CoSMo libwebrtc package has versioning and COMPONENTS
find_package(LibWebRTC 104 QUIET COMPONENTS H264 )
  if( NOT libwebrtc_FOUND )
    message( STATUS "No CoSMo package was found, searching for any webrtc package.")
    # A FindLibWebRTC package is also provided in the repository for self-compiled libwebrtc
    # this time we MUST find it, or fail, hence the REQUIRED
    find_package(LibWebRTC REQUIRED)
  endif()
message( STATUS "Looking for libwebrtc - found" )

# CoSMo package components allow for fine feature granularity, and to adjust
# the application build at configuration time.
if( WEBRTC_H264_FOUND )
  message( STATUS "This libwebrtc package was built with H264 support." )
else()
  # Pass configuration items to the Code layer so as to e.g. not list
  # H.264 video codec in the UI if it is not available in libwebrtc
  # we could use a .h file, but here we are using precompiler instead
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_WEBRTC_H264")
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -DDISABLE_WEBRTC_H264")
endif()
if( WEBRTC_EXT_OPENSSL_FOUND )
  message( STATUS "This libwebrtc package was built against openssl." )
endif()
if( WEBRTC_SFRAME_FOUND )
  message( STATUS "This libwebrtc package was build with SFrame end-to-end encryption." )
endif()

# --- Sources

set(obs-outputs_webrtc_HEADERS
	AudioDeviceModuleWrapper.h
	millicast-stream.h
	webrtc-custom-stream.h
  obsWebrtcAudioSource.h
	SDPModif.h
	VideoCapturer.h
	WebRTCStream.h
  useless_cc_network_controller.h
  useless_network_factory.h
  webrtc_pc_factory_di_helpers.h
  )
set(obs-outputs_webrtc_SOURCES
	AudioDeviceModuleWrapper.cpp
	millicast-stream.cpp
	webrtc-custom-stream.cpp
  obsWebrtcAudioSource.cpp
	VideoCapturer.cpp
	WebRTCStream.cpp
  useless_cc_network_controller.cpp
  useless_network_factory.cpp
	)

if(NOT WIN32)
	set_source_files_properties(${obs-outputs_webrtc_SOURCES} PROPERTIES
		COMPILE_FLAGS "-fvisibility=hidden")
endif()

#-------------------------------------------------------------------------------
# - Millicast WS Signalling library
#-------------------------------------------------------------------------------

set( MC_SIGNAL_LIB_DIR websocket-client )
add_subdirectory(${MC_SIGNAL_LIB_DIR})

#-------------------------------------------------------------------------------
# - RTMP Support
#-------------------------------------------------------------------------------

if(NOT DEFINED ENABLE_RTMPS)
  set(ENABLE_RTMPS
      AUTO
      CACHE STRING "Enable RTMPS support with mbedTLS" FORCE)
  set_property(CACHE ENABLE_RTMPS PROPERTY STRINGS AUTO ON OFF)
endif()

option(ENABLE_STATIC_MBEDTLS "Enable statically linking mbedTLS into binary"
       OFF)
mark_as_advanced(ENABLE_STATIC_MBEDTLS)

add_library(obs-outputs MODULE)
add_library(OBS::outputs ALIAS obs-outputs)

target_sources(
  obs-outputs
  PRIVATE obs-outputs.c
          obs-output-ver.h
          flv-mux.c
          flv-mux.h
          flv-output.c
          net-if.c
          net-if.h
          null-output.c
          rtmp-helpers.h
          rtmp-stream.c
          rtmp-stream.h
          rtmp-windows.c
          rtmp-av1.c
          rtmp-av1.h
          utils.h
          librtmp/amf.c
          librtmp/amf.h
          librtmp/bytes.h
          librtmp/cencode.c
          librtmp/cencode.h
          librtmp/handshake.h
          librtmp/hashswf.c
          librtmp/http.h
          librtmp/log.c
          librtmp/log.h
          librtmp/md5.c
          librtmp/md5.h
          librtmp/parseurl.c
          librtmp/rtmp.c
          librtmp/rtmp.h
          librtmp/rtmp_sys.h
          "$<$<BOOL:${ENABLE_HEVC}>:rtmp-hevc.c>"
          "$<$<BOOL:${ENABLE_HEVC}>:rtmp-hevc.h>"
          ${obs-outputs_webrtc_HEADERS}
          ${obs-outputs_webrtc_SOURCES})

target_include_directories(obs-outputs PRIVATE "${CMAKE_SOURCE_DIR}/UI/obs-frontend-api")
target_link_libraries(obs-outputs PRIVATE OBS::libobs MbedTLS::MbedTLS ZLIB::ZLIB
                                          obs-frontend-api
                                          websocketclient
                                          ${WEBRTC_LIBRARIES})

target_compile_definitions(obs-outputs PRIVATE USE_MBEDTLS CRYPTO)

set_target_properties(obs-outputs PROPERTIES FOLDER "plugins" PREFIX "")

if(OS_WINDOWS)
  target_link_libraries(obs-outputs PRIVATE OBS::w32-pthreads ws2_32 winmm Iphlpapi crypt32)
  target_link_options(obs-outputs PRIVATE /IGNORE:4098 /IGNORE:4099)

  configure_file(cmake/windows/obs-module.rc.in obs-outputs.rc)

  target_sources(obs-outputs PRIVATE obs-outputs.rc)

  if(MSVC)
    target_link_libraries(obs-outputs PRIVATE OBS::w32-pthreads)
    target_link_options(obs-outputs PRIVATE "LINKER:/IGNORE:4098"
                        "LINKER:/IGNORE:4099")
  endif()

  target_link_libraries(obs-outputs PRIVATE ws2_32 winmm Iphlpapi)
elseif(OS_MACOS)
  find_library(FOUNDATION_FRAMEWORK Foundation)
  find_library(SECURITY_FRAMEWORK Security)
  mark_as_advanced(FOUNDATION_FRAMEWORK SECURITY_FRAMEWORK)

  target_link_libraries(obs-outputs PRIVATE ${FOUNDATION_FRAMEWORK} ${SECURITY_FRAMEWORK})

  target_compile_options(
    obs-outputs
    PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-Wno-error=unused-parameter;-Wno-error=deprecated-declarations>
  )
endif()

include(cmake/ftl.cmake)

set_target_properties_obs(obs-outputs PROPERTIES FOLDER plugins PREFIX "")

# --- Compiler extra settings

if(WIN32 AND MSVC)
	if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		target_compile_options(obs-outputs PUBLIC "/MTd")
	else()
		target_compile_options(obs-outputs PUBLIC "/MT")
	endif()
endif()
