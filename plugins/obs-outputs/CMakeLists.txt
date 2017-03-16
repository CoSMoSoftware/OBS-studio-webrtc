project(obs-outputs)

option(USE_SSL "Enable rtmps support with OpenSSL" OFF)

if (USE_SSL)
	find_package(SSL QUIET)
	find_package(ZLIB QUIET)
endif()

if (SSL_FOUND AND ZLIB_FOUND)
	add_definitions(-DCRYPTO -DUSE_OPENSSL)
	include_directories(${SSL_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
else()
	if (USE_SSL)
		message(WARNING "SSL enabled by user, but OpenSSL was not found")
	endif()
	unset(SSL_LIBRARIES)
	unset(ZLIB_LIBRARIES)
	add_definitions(-DNO_CRYPTO)
endif()


if(WIN32)
	set(obs-outputs_PLATFORM_DEPS
		ws2_32
		winmm
		Iphlpapi)
endif()

if(MSVC)
	set(obs-outputs_PLATFORM_DEPS
		${obs-outputs_PLATFORM_DEPS}
		w32-pthreads)
endif()

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
	librtmp/md5.c
	librtmp/parseurl.c
	librtmp/rtmp.c)

if(NOT WIN32)
	set_source_files_properties(${obs-outputs_librtmp_SOURCES} PROPERTIES
		COMPILE_FLAGS "-fvisibility=hidden")
endif()

set(obs-outputs_HEADERS
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
	librtmp)
set(obs-outputs_SOURCES
	obs-outputs.c
	rtmp-stream.c
	rtmp-windows.c
	flv-output.c
	flv-mux.c
	AudioDeviceModuleWrapper.cpp
	VideoCapturer.cpp
	WebRTCStream.cpp
	net-if.c)

# --- Find libwebrtc or fail early
find_package( LibWebRTC REQUIRED )
include( ${LIBWEBRTC_USE_FILE} )

add_library(obs-outputs MODULE
	${obs-outputs_SOURCES}
	${obs-outputs_HEADER}
	${obs-outputs_librtmp_SOURCES}
	${obs-outputs_librtmp_HEADERS})
target_link_libraries(obs-outputs
	libobs
	${SSL_LIBRARIES}
	${ZLIB_LIBRARIES}
	${obs-outputs_PLATFORM_DEPS}
	${LIBWEBRTC_LIBRARIES})

install_obs_plugin_with_data(obs-outputs data)
