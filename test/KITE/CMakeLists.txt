message( STATUS "KITE Integration in progress." )

# A little standalone way of testing obs,
# from both build dir and install dir
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
  cmake_minimum_required( VERSION 3.14 FATAL_ERROR )
  project( standaloneKITEtestingOfOBS )
  if( NOT OBS_EXE_OR_APP_PATHNAME )
    message( STATUS "missing arg.\nUsage: mkdir BUILD && cd BUILD && cmake -DOBS_EXE_OR_APP_PATHNAME=<pathname> .." )
    return()
  endif()
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/Modules/")
  find_package( KITE COMPONENTS Appium REQUIRED )
  include( add_KITE_tests_MACRO )
  include( CTEST )
  add_KITE_tests( OBS-KITE-Test ${OBS_EXE_OR_APP_PATHNAME} )
endif()
