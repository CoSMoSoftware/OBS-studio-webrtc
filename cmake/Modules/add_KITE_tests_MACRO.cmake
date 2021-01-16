# Copyright 2019 ~ 2021 Alex Gouaillard @ CoSMo Software
# ----------------------------------------------------------------
# only tests for desktop clients for now
# "InstallInBuildTree" target need to exists
# This needs to be called in a CMakeLists.txt at the root of the
# KITE tests dir.
# ----------------------------------------------------------------

# rotate the targets at every call of the macro
set( _last_target InstallInBuildTree )

# the argument is the full path to the app bundle on mac,
# or the path of the executable on linux or on windows
macro( add_KITE_tests _app_name_or_exe_path )

  # should check it exists
  set( APP_EXE "${_app_name_or_exe_path}" )
  get_filename_component( APP_WORKING_DIR ${_app_name_or_exe_path} DIRECTORY )

  # --- Define OS specific KITE Variables
  if( WIN32 )
    set( OS WINDOWS )
    set( _appium_start_CMD ./${_app_name}-launchAppiumNode.bat )
    set( _appium_stop_CMD  taskkill /F /IM appium.exe /T )
    configure_file( 
      ${CMAKE_CURRENT_SOURCE_DIR}/launchAppiumNode.bat.in
      ${CMAKE_CURRENT_BINARY_DIR}/${_app_name}-launchAppiumNode.bat )
  elseif( APPLE )
    set( OS MAC )
    set( _appium_start_CMD ./${_app_name}-launchAppiumNode.sh )
    set( _appium_stop_CMD  pkill -9 -f appium )
    configure_file( 
      ${CMAKE_CURRENT_SOURCE_DIR}/launchAppiumNode.sh.in
      ${CMAKE_CURRENT_BINARY_DIR}/${_app_name}-launchAppiumNode.sh )
  elseif( UNIX AND NOT APPLE )
    set( OS LINUX )
    message( WARNING "Linux is not fully supported yet." )
  endif()

  # --- Start Appium Node
  # Configure appium node configuration file:
  configure_file( 
    ${CMAKE_CURRENT_SOURCE_DIR}/configs/appium/node.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/${_app_name}-node.json )
  add_test(
    NAME              ${_app_name}-LaunchAppiumNode
    COMMAND           ${_appium_start_CMD}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
  set_tests_properties( ${_app_name}-LaunchAppiumNode PROPERTIES
    DEPENDS ${_last_target}
    FIXTURES_SETUP ${_app_name}-appium
    )

  # --- Compile KITE Tests
  set( _my_test_file
    ${CMAKE_CURRENT_SOURCE_DIR}/configs/rtmp.config.json )
  configure_file( ${_my_test_file}.in ${_my_test_file} )
  add_test(
    NAME              ${_app_name}-KiteCompile
    COMMAND           ${KITE_C_CMD}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  set_tests_properties( ${_app_name}-KiteCompile PROPERTIES
    DEPENDS ${_app_name}-LaunchAppiumNode
    FIXTURES_REQUIRED ${_app_name}-appium
    )

  # --- Run KITE Tests
  add_test(
    NAME              ${_app_name}-KiteRun
    COMMAND           ${KITE_R_CMD} ${_my_test_file}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  set_tests_properties( ${_app_name}-KiteRun PROPERTIES
    DEPENDS ${_app_name}-KiteCompile
    FIXTURES_REQUIRED ${_app_name}-appium
    )

  # --- Shutdown appium node
  add_test(
    NAME              ${_app_name}-killAppiumNode
    COMMAND           ${_appium_stop_CMD}
    )
  set_tests_properties( ${_app_name}-killAppiumNode PROPERTIES
    DEPENDS ${_app_name}-KiteRun
    FIXTURES_CLEANUP ${_app_name}-appium
    )

  # make sure that two calls to this macro results in tests that
  # are never run in parallel. Note that the lock name is global
  set_tests_properties(
    ${_app_name}-LaunchAppiumNode
    ${_app_name}-KiteCompile
    ${_app_name}-KiteRun
    ${_app_name}-killAppiumNode
    PROPERTIES RESOURCE_LOCK appium_lock
    )

  set( _last_target ${_app_name}-killAppiumNode ) 

endmacro()

