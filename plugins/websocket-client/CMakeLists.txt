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
find_package(libwebrtc REQUIRED)
if( WIN32 )
  if( NOT OPENSSL_ROOT_DIR )
    set (OPENSSL_ROOT_DIR "C:/Program Files (x86)/OpenSSL")
  endif()
  # find_package(OpenSSL REQUIRED)
  # install(FILES ${OPENSSL_ROOT_DIR}/bin/libcrypto-1_1-x64.dll 
  #         DESTINATION ${CMAKE_INSTALL_BINDIR}/64bit/)
  # install(FILES ${OPENSSL_ROOT_DIR}/bin/libssl-1_1-x64.dll 
          # DESTINATION ${CMAKE_INSTALL_BINDIR}/64bit/)
elseif( APPLE )
  if( NOT OPENSSL_ROOT_DIR )
    set( OPENSSL_ROOT_DIR    /usr/local/ )
  endif()
  set( OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include )
  set( OPENSSL_LIBRARIES   
    ${OPENSSL_ROOT_DIR}/lib/libssl.dylib
    ${OPENSSL_ROOT_DIR}/lib/libcrypto.dylib
    )
endif()

#include_directories("C:/Program Files/libwebrtc/include/")

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
# if( WIN32 )
  target_link_libraries( websocketclient
    ${WEBRTC_LIBRARIES}
#    "C:/Program Files/libwebrtc/lib/libcrypto.lib"
#    "C:/Program Files/libwebrtc/lib/libssl.lib"
  )
# elseif( APPLE )
  # add_library( ssl UNKNOWN IMPORTED)
  # set_property(TARGET ssl PROPERTY IMPORTED_LOCATION "/usr/local/opt/openssl/lib/libssl.dylib")
  # add_library( crypto UNKNOWN IMPORTED)
  # set_property(TARGET crypto PROPERTY IMPORTED_LOCATION "/usr/local/opt/openssl/lib/libcrypto.dylib")
  # target_link_libraries( websocketclient
  #   ssl
  #   crypto
  #   )
# endif()
install_obs_plugin(websocketclient)
