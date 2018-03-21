project(websocketclient)

# -----------------------------------------------------------------------
# Global set up
#
set(CMAKE_CXX_STANDARD 11)
if( WIN32 )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503 /wd4267 /wd4244 /wd4996")
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#------------------------------------------------------------------------
# OpenSSL included in libwebrtc
#
find_package(libwebrtc REQUIRED)

#------------------------------------------------------------------------
# Asio
#
set( ASIO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio )
add_subdirectory( ${ASIO_ROOT} )
include_directories( ${ASIO_ROOT}/include )

#------------------------------------------------------------------------
# WebSocket
#
set( websocketclient_SOURCES WebsocketClientImpl.cpp )
if( WIN32 )
  set( websocketclient_SOURCES ${websocketclient_SOURCES} dllmain.cpp )
  add_definitions( -DWEBSOCKETCLIENT_EXPORTS )
endif()

set( websocketclient_HEADERS
  json.hpp
  WebsocketClientImpl.h
  WebsocketClient.h
  )

add_library( websocketclient SHARED
  ${websocketclient_SOURCES}
  ${websocketclient_HEADERS}
  )

  target_link_libraries( websocketclient
    ${WEBRTC_LIBRARIES}
  )
install_obs_plugin(websocketclient)
