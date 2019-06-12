project(websocketclient)

# -----------------------------------------------------------------------
# Global set up
#
set(CMAKE_CXX_STANDARD 11)
if( WIN32 )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503 /wd4267 /wd4244 /wd4996")
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/restclient-cpp/include)

find_package(LibObs REQUIRED)
include_directories(${LIBOBS_INCLUDE_DIRS})

#------------------------------------------------------------------------
# OpenSSL
#
find_package(OpenSSL 1.1 REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

#------------------------------------------------------------------------
# Asio
#
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include )
# NOTE ALEX: this is project specific and not using the standard module
include_directories( ${CURL_INCLUDE_DIR} )

#------------------------------------------------------------------------
# WebSocket
#
# NOTE ALEX: to accomodate all vendors who want a branded interface and
# support only their service, we shoul dmake it modular here.
set( websocketclient_SOURCES 
  JanusWebsocketClientImpl.cpp
  EvercastWebsocketClientImpl.cpp
  MillicastWebsocketClientImpl.cpp
  WebsocketClient.cpp
  restclient-cpp/source/connection.cc
  restclient-cpp/source/helpers.cc
  restclient-cpp/source/restclient.cc
  )
if( WIN32 )
  set( websocketclient_SOURCES ${websocketclient_SOURCES} dllmain.cpp )
endif()

add_definitions( -DWEBSOCKETCLIENT_EXPORTS )

set( websocketclient_HEADERS
  json.hpp
  EvercastWebsocketClientImpl.h
  JanusWebsocketClientImpl.h
  MillicastWebsocketClientImpl.h
  WebsocketClient.h
  )

add_library( websocketclient SHARED
  ${websocketclient_SOURCES}
  ${websocketclient_HEADERS}
  )

target_link_libraries( websocketclient
  # NOTE ALEX: this is project specific and not using the standard module
  ${CURL_LIB}
  ${OPENSSL_LIBRARIES}
  # ${OPENSSL_ROOT_DIR}/lib/libssl.dylib
  # ${OPENSSL_ROOT_DIR}/lib/libcrypto.dylib
  )

install_obs_plugin(websocketclient)
