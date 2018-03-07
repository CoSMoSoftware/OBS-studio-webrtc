project(obs-outputs)

# --- Find libwebrtc or fail early
find_package(libwebrtc REQUIRED)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ftl-sdk/CMakeLists.txt")
	find_package(Libcurl REQUIRED)

	add_definitions(-DFTL_STATIC_COMPILE)

	include_directories(${LIBCURL_INCLUDE_DIRS})

	set(ftl_SOURCES
		ftl-stream.c
		ftl-sdk/libftl/hmac/hmac.c
		ftl-sdk/libftl/hmac/sha2.c
		ftl-sdk/libftl/ftl-sdk.c
		ftl-sdk/libftl/handshake.c
		ftl-sdk/libftl/ingest.c
		ftl-sdk/libftl/ftl_helpers.c
		ftl-sdk/libftl/media.c
		ftl-sdk/libftl/gettimeofday/gettimeofday.c
		ftl-sdk/libftl/logging.c)
	set(ftl_HEADERS
		ftl-sdk/libftl/hmac/hmac.h
		ftl-sdk/libftl/hmac/sha2.h
		ftl-sdk/libftl/ftl.h
		ftl-sdk/libftl/ftl_private.h)
	set(ftl_IMPORTS
		${OBS_JANSSON_IMPORT}
		${LIBCURL_LIBRARIES})

	if (WIN32)
		list(APPEND ftl_SOURCES
			ftl-sdk/libftl/win32/socket.c
			ftl-sdk/libftl/gettimeofday/gettimeofday.c
			ftl-sdk/libftl/win32/threads.c)
		list(APPEND ftl_HEADERS
			ftl-sdk/libftl/gettimeofday/gettimeofday.h
			ftl-sdk/libftl/win32/threads.h)

		include_directories(ftl-sdk/libftl/win32)
	else()
		list(APPEND ftl_SOURCES
			ftl-sdk/libftl/posix/socket.c
			ftl-sdk/libftl/posix/threads.c)
		list(APPEND ftl_HEADERS
			ftl-sdk/libftl/posix/threads.h)

		include_directories(ftl-sdk/libftl/posix)
	endif()

	include_directories(ftl-sdk/libftl)

	set(COMPILE_FTL TRUE)
else()
	set(COMPILE_FTL FALSE)
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/obs-outputs-config.h.in"
	"${CMAKE_BINARY_DIR}/plugins/obs-outputs/config/obs-outputs-config.h")

include_directories("${CMAKE_BINARY_DIR}/plugins/obs-outputs/config")

if(WIN32)
set( obs-outputs_PLATFORM_DEPS  
  ${WEBRTC_LIBRARIES}
  ws2_32
  winmm
  Iphlpapi
)
  if(MSVC)
    set( obs-outputs_PLATFORM_DEPS ${obs-outputs_PLATFORM_DEPS} w32-pthreads )
  endif()
  elseif( APPLE )
    set( obs-outputs_PLATFORM_DEPS ${WEBRTC_LIBRARIES} )
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
  null-output.c
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