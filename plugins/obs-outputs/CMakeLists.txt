#-------------------------------------------------------------------------------
# - OBS outputs
#
# * RTMP output
# * WEBRTC output (WHIP signalling)
# * WEBRTC output (Millicast signalling)
#-------------------------------------------------------------------------------

project(obs-outputs)
include_directories("${CMAKE_BINARY_DIR}/plugins/obs-outputs/config")
include_directories("${CMAKE_SOURCE_DIR}/libobs")

#-------------------------------------------------------------------------------
# - Millicast WS Signalling library
#-------------------------------------------------------------------------------

set( MC_SIGNAL_LIB_DIR websocket-client )
add_subdirectory(${MC_SIGNAL_LIB_DIR})

#-------------------------------------------------------------------------------
# WebRTC
#-------------------------------------------------------------------------------

if(WIN32)
	set(WEBRTC_USE_FILE_INCLUDED 0)
endif()

# --- libwebrtc

message( STATUS "Looking for libwebrtc" )

# CoSMo libwebrtc package has versioning and COMPONENTS
find_package(LibWebRTC 90 QUIET COMPONENTS H264 )
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
        # H.264 video codec In the UI if it is not available in libwebrtc
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
       )
set(obs-outputs_webrtc_SOURCES
	AudioDeviceModuleWrapper.cpp
	millicast-stream.cpp
	webrtc-custom-stream.cpp
        obsWebrtcAudioSource.cpp
	VideoCapturer.cpp
	WebRTCStream.cpp
	)

if(NOT WIN32)
	set_source_files_properties(${obs-outputs_webrtc_SOURCES} PROPERTIES
		COMPILE_FLAGS "-fvisibility=hidden")
endif()

#-------------------------------------------------------------------------------
# - RTMP Support
#-------------------------------------------------------------------------------

# --- Secure RTMP if SSL implementation available on the build host

set(WITH_RTMPS AUTO CACHE STRING "Enable RTMPS support with mbedTLS")
set_property(CACHE WITH_RTMPS PROPERTY STRINGS AUTO ON OFF)

option(STATIC_MBEDTLS "Statically link mbedTLS into binary" OFF)

if (WITH_RTMPS STREQUAL "AUTO")
	find_package(MbedTLS)
	find_package(ZLIB)
	if (NOT MBEDTLS_FOUND OR NOT ZLIB_FOUND)
		set(WITH_RTMPS "OFF")
		message(WARNING "mbedTLS or zlib was not found, RTMPS will be auto-disabled")
	endif()
endif()

if (WITH_RTMPS)
	find_package(MbedTLS REQUIRED)
	find_package(ZLIB REQUIRED)
	add_definitions(-DCRYPTO -DUSE_MBEDTLS)
	include_directories(${MBEDTLS_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
else()
	add_definitions(-DNO_CRYPTO)
endif()

# --- Sources

set(obs-outputs_librtmp_HEADERS
	librtmp/amf.h
	librtmp/bytes.h
	librtmp/cencode.h
	librtmp/dh.h
	librtmp/dhgroups.h
	librtmp/handshake.h
	librtmp/http.h
	librtmp/log.h
	librtmp/md5.h
	librtmp/rtmp.h
	librtmp/rtmp_sys.h)
set(obs-outputs_librtmp_SOURCES
	librtmp/amf.c
	librtmp/cencode.c
	librtmp/hashswf.c
	librtmp/log.c
	# librtmp/md5.c
	librtmp/parseurl.c
	librtmp/rtmp.c)

if(NOT WIN32)
	set_source_files_properties(${obs-outputs_librtmp_SOURCES} PROPERTIES
		COMPILE_FLAGS "-fvisibility=hidden")
endif()

#-------------------------------------------------------------------------------
# - Library target definition
#-------------------------------------------------------------------------------

set(COMPILE_FTL FALSE)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/obs-outputs-config.h.in"
	"${CMAKE_BINARY_DIR}/plugins/obs-outputs/config/obs-outputs-config.h")
set(obs-outputs_HEADERS
	"${CMAKE_BINARY_DIR}/plugins/obs-outputs/config/obs-outputs-config.h"
	obs-output-ver.h
	rtmp-helpers.h
	rtmp-stream.h
	net-if.h)
set(obs-outputs_SOURCES
	obs-outputs.c
	null-output.c
	rtmp-stream.c
	rtmp-windows.c
	net-if.c
	flv-output.c
	flv-mux.c)

if(WIN32)
	set(MODULE_DESCRIPTION "OBS output module")
	configure_file(
                ${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in
                obs-outputs.rc)
	list(APPEND obs-outputs_SOURCES
		obs-outputs.rc)
endif()

add_library(obs-outputs MODULE
	${obs-outputs_SOURCES}
	${obs-outputs_HEADERS}
	${obs-outputs_librtmp_SOURCES}
	${obs-outputs_librtmp_HEADERS}
	${obs-outputs_webrtc_SOURCES}
	${obs-outputs_webrtc_HEADERS})

target_include_directories(obs-outputs
	PUBLIC  "${WEBRTC_INCLUDE_DIR}"
	PUBLIC  "${MC_SIGNAL_LIB_DIR}")

#-------------------------------------------------------------------------------
# - System Libs and linking
#-------------------------------------------------------------------------------

if(WIN32)
	set(obs-outputs_PLATFORM_DEPS
		ws2_32
		winmm
		Iphlpapi)

	if (WITH_RTMPS OR (WITH_RTMPS STREQUAL "AUTO"))
		SET(obs-outputs_PLATFORM_DEPS
			${obs-outputs_PLATFORM_DEPS}
			crypt32)
	endif()
endif()

if(MSVC)
	set(obs-outputs_PLATFORM_DEPS
		${obs-outputs_PLATFORM_DEPS}
		w32-pthreads)
endif()

if(APPLE AND (WITH_RTMPS OR (WITH_RTMPS STREQUAL "AUTO")))
	find_library(FOUNDATION_FRAMEWORK Foundation)
	find_library(SECURITY_FRAMEWORK Security)

	set(obs-outputs_PLATFORM_DEPS
		${obs-outputs_PLATFORM_DEPS}
		${FOUNDATION_FRAMEWORK}
		${SECURITY_FRAMEWORK})
endif()

target_link_libraries(obs-outputs
	libobs
	websocketclient
	${MBEDTLS_LIBRARIES}
	${WEBRTC_LIBRARIES}
	${ZLIB_LIBRARIES}
	${ftl_IMPORTS}
	${obs-outputs_PLATFORM_DEPS})
set_target_properties(obs-outputs PROPERTIES FOLDER "plugins")

# --- Compiler extra settings

if(WIN32 AND MSVC)
	if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		target_compile_options(obs-outputs PUBLIC "/MTd")
	else()
		target_compile_options(obs-outputs PUBLIC "/MT")
	endif()
endif()

# --- build tree install

install_obs_plugin_with_data(obs-outputs data)
