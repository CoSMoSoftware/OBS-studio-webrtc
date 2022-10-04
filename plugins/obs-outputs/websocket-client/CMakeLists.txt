# Copyright Dr. Alex. Gouaillard (2015, 2020)

project(websocketclient)

#------------------------------------------------------------------------
# Global setup
#
# NOTE ALEX: we should check that we are not overriding thsi setting here
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503 /wd4267 /wd4244 /wd4996")
endif()

#------------------------------------------------------------------------
# JSON
#
set(JSON_BuildTests OFF CACHE INTERNAL "")
# set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(nlohmann_json)

#------------------------------------------------------------------------
# OpenSSL
#
find_package(OpenSSL 1.1 REQUIRED)
message(STATUS "OPENSSL_INCLUDE_DIR:    ${OPENSSL_INCLUDE_DIR}"   )
message(STATUS "OPENSSL_CRYPTO_LIBRARY: ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "OPENSSL_SSL_LIBRARY:    ${OPENSSL_SSL_LIBRARY}"   )
message(STATUS "OPENSSL_VERSION:        ${OPENSSL_VERSION}"       )

#------------------------------------------------------------------------
# curl
#
find_package(CURL REQUIRED)

#------------------------------------------------------------------------
# WebSocket
#
set(websocketclient_SOURCES
	WebsocketClient.cpp
	MillicastWebsocketClientImpl.cpp
	CustomWebrtcImpl.cpp
	restclient-cpp/source/connection.cc
	restclient-cpp/source/helpers.cc
	restclient-cpp/source/restclient.cc)


set(websocketclient_HEADERS
	WebsocketClient.h
	MillicastWebsocketClientImpl.h
	CustomWebrtcImpl.h)

if(WIN32)
	set(websocketclient_SOURCES ${websocketclient_SOURCES} dllmain.cpp)
  add_definitions(-DWEBSOCKETCLIENT_EXPORTS)
	add_library(websocketclient SHARED)
else()
  add_library(websocketclient STATIC)
endif()

target_sources(
	websocketclient
	PRIVATE ${websocketclient_SOURCES}
					${websocketclient_HEADERS})

target_link_libraries(websocketclient
  PUBLIC libobs
	PRIVATE CURL::libcurl
	PRIVATE OpenSSL::SSL
	PRIVATE nlohmann_json::nlohmann_json)

target_include_directories(websocketclient
	PRIVATE ${OPENSSL_INCLUDE_DIR}
					${CMAKE_CURRENT_SOURCE_DIR}
					${CMAKE_CURRENT_SOURCE_DIR}/restclient-cpp/include
					${CMAKE_CURRENT_SOURCE_DIR}/websocketpp
					${CMAKE_SOURCE_DIR}/libobs
					${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include)

if(WIN32)
	setup_plugin_target(websocketclient)
endif()