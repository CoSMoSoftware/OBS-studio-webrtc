set (CMAKE_CXX_STANDARD 11)

if( WIN32 )
  set (OPENSSL_ROOT_DIR "C:/Program Files (x86)/OpenSSL")
  set (asio_DIR "C:\Users\Evasyst-1\Documents\obs-studio-1\plugins\websocket-client\third_party\asio" )
endif()

find_package( OpenSSL REQUIRED )

find_package(asio REQUIRED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  )

set( websocketclient_SOURCES WebsocketClientImpl.cpp )
if( WIN32 )
  set( websocketclient_SOURCES
    ${websocketclient_SOURCES}
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
target_link_libraries(websocketclient asio::standalone)
target_link_libraries( websocketclient ${OPENSSL_LIBRARIES} )  
