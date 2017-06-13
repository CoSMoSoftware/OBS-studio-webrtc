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
# OpenSSL
#
if( WIN32 )
  if( NOT OPENSSL_ROOT_DIR )
    set (OPENSSL_ROOT_DIR "C:/Program Files (x86)/OpenSSL")
  endif()
endif()
find_package( OpenSSL REQUIRED )
include_directories(${OPENSSL_INCLUDE_DIR})

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
  set( websocketclient_SOURCES ${websocketclient_SOURCES}
    dllmain.cpp
  )
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



if(APPLE)
  find_package(OpenSSL REQUIRED)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if (WIN32)
  add_library( ssl UNKNOWN IMPORTED)
  set_property(TARGET ssl PROPERTY IMPORTED_LOCATION "/opt/local/lib/libssl.dylib")
  add_library( crypto UNKNOWN IMPORTED)
  set_property(TARGET crypto PROPERTY IMPORTED_LOCATION "/opt/local/lib/libcrypto.dylib")
endif()

target_link_libraries( websocketclient
  ${OPENSSL_LIBRARIES}
) 
install_obs_plugin_with_data(websocketclient data)
message( WARNING "DO NOT FORGET TO COPY THE websocketclient DLL OVER!!" ) 
