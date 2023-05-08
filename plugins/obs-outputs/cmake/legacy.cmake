project(obs-outputs)

if(NOT DEFINED ENABLE_RTMPS)
  set(ENABLE_RTMPS
      AUTO
      CACHE STRING "Enable RTMPS support with mbedTLS" FORCE)
  set_property(CACHE ENABLE_RTMPS PROPERTY STRINGS AUTO ON OFF)
endif()

option(ENABLE_STATIC_MBEDTLS "Enable statically linking mbedTLS into binary" OFF)
mark_as_advanced(ENABLE_STATIC_MBEDTLS)

add_library(obs-outputs MODULE)
add_library(OBS::outputs ALIAS obs-outputs)

target_sources(
  obs-outputs
  PRIVATE obs-outputs.c
          obs-output-ver.h
          flv-mux.c
          flv-mux.h
          flv-output.c
          net-if.c
          net-if.h
          null-output.c
          rtmp-helpers.h
          rtmp-stream.c
          rtmp-stream.h
          rtmp-windows.c
          rtmp-av1.c
          rtmp-av1.h
          utils.h
          librtmp/amf.c
          librtmp/amf.h
          librtmp/bytes.h
          librtmp/cencode.c
          librtmp/cencode.h
          librtmp/handshake.h
          librtmp/hashswf.c
          librtmp/http.h
          librtmp/log.c
          librtmp/log.h
          librtmp/md5.c
          librtmp/md5.h
          librtmp/parseurl.c
          librtmp/rtmp.c
          librtmp/rtmp.h
          librtmp/rtmp_sys.h)

if(ENABLE_HEVC)
  target_sources(obs-outputs PRIVATE rtmp-hevc.c rtmp-hevc.h)
endif()

target_link_libraries(obs-outputs PRIVATE OBS::libobs)

set_target_properties(obs-outputs PROPERTIES FOLDER "plugins" PREFIX "")

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS output module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in obs-outputs.rc)

  target_sources(obs-outputs PRIVATE obs-outputs.rc)

  if(MSVC)
    target_link_libraries(obs-outputs PRIVATE OBS::w32-pthreads)
    target_link_options(obs-outputs PRIVATE "LINKER:/IGNORE:4098" "LINKER:/IGNORE:4099")
  endif()

  target_link_libraries(obs-outputs PRIVATE ws2_32 winmm Iphlpapi)
endif()

if(ENABLE_RTMPS STREQUAL "AUTO" OR ENABLE_RTMPS STREQUAL "ON")
  find_package(MbedTLS)
  find_package(ZLIB)
  if(NOT MBEDTLS_FOUND OR NOT ZLIB_FOUND)
    if(ENABLE_RTMPS STREQUAL "ON")
      obs_status(FATAL_ERROR "mbedTLS or zlib not found, but required for RTMPS support.")
      return()
    else()
      obs_status(WARNING "mbedTLS or zlib was not found, RTMPS will be automatically disabled.")
      target_compile_definitions(obs-outputs PRIVATE NO_CRYPTO)
    endif()
  else()
    target_compile_definitions(obs-outputs PRIVATE USE_MBEDTLS CRYPTO)

    target_link_libraries(obs-outputs PRIVATE Mbedtls::Mbedtls ZLIB::ZLIB)

    if(OS_WINDOWS)
      target_link_libraries(obs-outputs PRIVATE crypt32)

    elseif(OS_MACOS)
      find_library(FOUNDATION_FRAMEWORK Foundation)
      find_library(SECURITY_FRAMEWORK Security)
      mark_as_advanced(FOUNDATION_FRAMEWORK SECURITY_FRAMEWORK)

      target_link_libraries(obs-outputs PRIVATE ${FOUNDATION_FRAMEWORK} ${SECURITY_FRAMEWORK})
      set_target_properties(obs-outputs PROPERTIES CXX_VISIBILITY_PRESET hidden)
      set_target_properties(obs-outputs PROPERTIES C_VISIBILITY_PRESET hidden)

    elseif(OS_POSIX)
      set_target_properties(obs-outputs PROPERTIES CXX_VISIBILITY_PRESET hidden)
      set_target_properties(obs-outputs PROPERTIES C_VISIBILITY_PRESET hidden)
    endif()
  endif()
else()
  target_compile_definitions(obs-outputs PRIVATE NO_CRYPTO)
endif()

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(FTL QUIET libftl)
endif()

if(FTL_FOUND)
  find_package(CURL REQUIRED)
  obs_status(ENABLED "ftl outputs (system ftl-sdk)")

  target_sources(obs-outputs PRIVATE ftl-stream.c)

  target_include_directories(obs-outputs PRIVATE ${FTL_INCLUDE_DIRS})

  target_link_libraries(obs-outputs PRIVATE ${FTL_LIBRARIES} CURL::libcurl)

  target_compile_features(obs-outputs PRIVATE c_std_11)

  target_compile_definitions(obs-outputs PRIVATE FTL_FOUND)

elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ftl-sdk/CMakeLists.txt")
  find_package(CURL REQUIRED)
  obs_status(ENABLED "ftl ouputs (bundled ftl-sdk)")

  target_compile_definitions(obs-outputs PRIVATE FTL_STATIC_COMPILE)

  target_compile_features(obs-outputs PRIVATE c_std_11)

  target_link_libraries(obs-outputs PRIVATE Jansson::Jansson CURL::libcurl)

  target_sources(
    obs-outputs
    PRIVATE ftl-stream.c
            ftl-sdk/libftl/ftl.h
            ftl-sdk/libftl/ftl_private.h
            ftl-sdk/libftl/hmac/hmac.c
            ftl-sdk/libftl/hmac/hmac.h
            ftl-sdk/libftl/hmac/sha2.c
            ftl-sdk/libftl/hmac/sha2.h
            ftl-sdk/libftl/ftl-sdk.c
            ftl-sdk/libftl/handshake.c
            ftl-sdk/libftl/ingest.c
            ftl-sdk/libftl/ftl_helpers.c
            ftl-sdk/libftl/media.c
            ftl-sdk/libftl/gettimeofday/gettimeofday.c
            ftl-sdk/libftl/logging.c)

  target_include_directories(obs-outputs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ftl-sdk/libftl)

  if(OS_WINDOWS)
    target_sources(
      obs-outputs PRIVATE ftl-sdk/libftl/gettimeofday/gettimeofday.c ftl-sdk/libftl/gettimeofday/gettimeofday.h
                          ftl-sdk/libftl/win32/socket.c ftl-sdk/libftl/win32/threads.c ftl-sdk/libftl/win32/threads.h)

    target_include_directories(obs-outputs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ftl-sdk/libftl/win32)
  elseif(OS_POSIX)
    target_sources(obs-outputs PRIVATE ftl-sdk/libftl/posix/socket.c ftl-sdk/libftl/posix/threads.c
                                       ftl-sdk/libftl/posix/threads.h)

    target_include_directories(obs-outputs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ftl-sdk/libftl/posix)
  endif()

  if(NOT MSVC)
    target_compile_options(
      obs-outputs
      PRIVATE -Wno-error=extra
              -Wno-error=sign-compare
              -Wno-error=incompatible-pointer-types
              -Wno-error=int-conversion
              -Wno-error=unused-parameter
              -Wno-error=deprecated-declarations
              "$<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-error=maybe-uninitialized>"
              "$<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-error=pointer-sign>")
    if((NOT CMAKE_C_COMPILER_ID STREQUAL "GNU") OR CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "10")
      target_compile_options(obs-outputs PRIVATE -Wno-error=enum-conversion)
    endif()
  endif()

  target_compile_definitions(obs-outputs PRIVATE FTL_FOUND)
endif()

setup_plugin_target(obs-outputs)
