if(NOT TARGET OBS::blake2)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/blake2" "${CMAKE_BINARY_DIR}/deps/blake2")
endif()

if(NOT TARGET OBS::w32-pthreads)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/w32-pthreads" "${CMAKE_BINARY_DIR}/deps/w32-pthreads")
endif()

find_package(MbedTLS)
find_package(Detours REQUIRED)

configure_file(cmake/windows/obs.rc.in obs.rc)

target_sources(
  obs
  PRIVATE obs.rc
          platform-windows.cpp
          win-dll-blocklist.c
          cmake/windows/obs.manifest
          update/crypto-helpers-mbedtls.cpp
          update/crypto-helpers.hpp
          update/shared-update.cpp
          update/shared-update.hpp
          update/update-helpers.cpp
          update/update-helpers.hpp
          update/update-window.cpp
          update/update-window.hpp
          update/win-update.cpp
          update/win-update.hpp)

target_link_libraries(obs PRIVATE crypt32 OBS::blake2 OBS::w32-pthreads MbedTLS::MbedTLS Detours::Detours)
target_compile_options(obs PRIVATE PSAPI_VERSION=2)
target_link_options(obs PRIVATE /IGNORE:4098 /IGNORE:4099)

add_library(obs-update-helpers INTERFACE)
add_library(OBS::update-helpers ALIAS obs-update-helpers)

target_sources(obs-update-helpers INTERFACE win-update/win-update-helpers.cpp win-update/win-update-helpers.hpp)
target_include_directories(obs-update-helpers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/win-update")

add_subdirectory(win-update/updater)

set_property(
  TARGET obs
  APPEND
  PROPERTY AUTORCC_OPTIONS --format-version 1)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT obs)
set_target_properties(
  obs
  PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DEBUGGER_COMMAND
    "${CMAKE_BINARY_DIR}/rundir/$<CONFIG>/$<$<BOOL:${OBS_WINDOWS_LEGACY_DIRS}>:bin/>$<TARGET_FILE_NAME:obs>"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/rundir/$<CONFIG>$<$<BOOL:${OBS_WINDOWS_LEGACY_DIRS}>:/bin>")
