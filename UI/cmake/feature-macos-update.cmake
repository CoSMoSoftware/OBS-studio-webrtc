include_guard(DIRECTORY)

if(NOT TARGET OBS::blake2)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/blake2" "${CMAKE_BINARY_DIR}/deps/blake2")
endif()

target_sources(obs PRIVATE update/crypto-helpers.hpp update/crypto-helpers-mac.mm update/shared-update.cpp
                                  update/shared-update.hpp update/update-helpers.cpp update/update-helpers.hpp)

target_link_libraries(obs PRIVATE "$<LINK_LIBRARY:FRAMEWORK,Security.framework>" OBS::blake2)
