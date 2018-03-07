cmake_minimum_required (VERSION 2.8.0)
enable_language(C)
project(libftl)

option(DISABLE_AUTO_INGEST "Set to TRUE to disable auto ingest feature which removes curl and jansson dependancies" FALSE)
MESSAGE( STATUS "FTL DISABLE_AUTO_INGEST: " ${DISABLE_AUTO_INGEST})

find_package(Threads REQUIRED)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (DISABLE_AUTO_INGEST)
  add_definitions(-DDISABLE_AUTO_INGEST)
endif()

# We will only try to include curl if we have auto ingest enabled.
if (NOT DISABLE_AUTO_INGEST)
  FIND_PACKAGE(CURL)
endif()
if (NOT CURL_FOUND AND NOT DISABLE_AUTO_INGEST)
  SET(CURL_DISABLE_NTLM ON CACHE BOOL "Disabling NTLM")
  SET(CURL_DISABLE_TELNET ON CACHE BOOL "Disabling Telnet")
  SET(CURL_DISABLE_LDAP ON CACHE BOOL "Disabling Ldap")
  SET(CURL_DISABLE_LDAPS ON CACHE BOOL "Disabling secure ldap")
  SET(BUILD_CURL_EXE OFF CACHE BOOL "Building libcurl")
  SET(HTTP_ONLY ON CACHE BOOL "using compiling HTTP")
  SET(BUILD_TESTING OFF CACHE BOOL "Not building Tests")
  add_subdirectory(libcurl)
  SET(CURL_INCLUDE_DIRS libcurl/include ${CMAKE_CURRENT_BINARY_DIR}/libcurl/include/curl)
  SET(CURL_LIBRARIES libcurl)
  include_directories(${CURL_INCLUDE_DIRS})
endif()

# We will only try to include lib jansson if auto ingest is enabled.
SET(JANSSON_LIBRARIES "")
if (NOT DISABLE_AUTO_INGEST)
  SET(JANSSON_BUILD_DOCS OFF CACHE BOOL "Jansson docs off")
  SET(JANSSON_WITHOUT_TESTS ON CACHE BOOL "Jansson build without tests")
  SET(JANSSON_EXAMPLES OFF CACHE BOOL "Jansson disable examples")
  SET(USE_WINDOWS_CRYPTOAPI off)
  add_subdirectory(libjansson)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/libjansson/include)
  SET(JANSSON_LIBRARIES jansson)
endif()

if (WIN32)
  set(FTL_PLATFORM_FILES  ftl_app/win32/xgetopt.c
                          ftl_app/win32/xgetopt.h
                          ftl_app/win32/ctrlc_handler.c)
  #set(FTL_PLATFORM_LIBS kernel32 user32 gdi32 advapi32 )
  #set(FTL_PLATFORM_LIBS ws2_32 )
  set(FTLSDK_PLATFORM_FILES libftl/win32/socket.c
                            libftl/win32/socket.h
                            libftl/win32/threads.c
                            libftl/win32/threads.h)
  include_directories(libftl/win32)
else()
  set(FTL_PLATFORM_FILES ftl_app/posix/ctrlc_handler.c)
  set(FTLSDK_PLATFORM_FILES libftl/posix/socket.c
                            libftl/posix/socket.h
                            libftl/posix/threads.c
                            libftl/posix/threads.h)
  include_directories(libftl/posix)
endif()

add_library(ftl SHARED libftl/hmac/hmac.c
                       libftl/hmac/hmac.h
                       libftl/hmac/sha2.c
                       libftl/hmac/sha2.h
                       libftl/gettimeofday/gettimeofday.c
                       libftl/gettimeofday/gettimeofday.h
                       libftl/ftl-sdk.c
                       libftl/handshake.c
                       libftl/ingest.c
                       libftl/ftl_helpers.c
                       libftl/media.c
                       libftl/logging.c
                       libftl/ftl.h
                       libftl/ftl_private.h
                       ${FTLSDK_PLATFORM_FILES})
include_directories(libftl libftl/gettimeofday)

set_target_properties(ftl PROPERTIES VERSION "0.5.0")
set_target_properties(ftl PROPERTIES SOVERSION 0)

target_link_libraries(ftl ${CURL_LIBRARIES} ${JANSSON_LIBRARIES})

if(WIN32)
  target_link_libraries(ftl ws2_32)
endif()

add_executable(ftl_app
               ftl_app/main.c
               ftl_app/main.h
               ftl_app/file_parser.c
               ftl_app/file_parser.h			   
               ftl_app/gettimeofday.c
               ftl_app/gettimeofday.h
               ftl_app/bitstream.c
               ftl_app/bitstream.h
               ftl_app/cavlc.c
               ftl_app/cavlc.h
               ftl_app/decode.c
               ftl_app/decode.h
               ftl_app/nalu.c
               ftl_app/nalu.h
               ftl_app/utils.c
               ftl_app/utils.h
               ftl_app/dec_obj.h
               ${FTL_PLATFORM_FILES})

target_link_libraries(ftl_app ftl ${CURL_LIBRARIES} ${JANSSON_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${FTL_PLATFORM_LIBS})
target_include_directories(ftl_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ftl_app)

# Install rules
install(TARGETS ftl DESTINATION lib)