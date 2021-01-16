# Copyright 2020 Dr Alex Gouaillard @ CoSMo

# so WIP that I should be ashamed.

#
# Defines:
#   KITE_HOME
#   KITE_PATH
#   KITE_R_CMD
#   KITE_C_CMD
#
# Does not support yet, but should eventually
#   KITE_DIR
#   VERSIONS
#

#-------------------------------------------------------------------------------
# Find main installation.
# Should use local variables for that

# --- Find KITE home/root directory

# KITE_HOME variable passed through cmake gets priority
if( NOT KITE_HOME )
  # second option is environement variable
  if( DEFINED ENV{KITE_HOME} )
    set( KITE_HOME $ENV{KITE_HOME} )
    message( STATUS "KITE_HOME set from ENV: ${KITE_HOME}" )
  else()
    set( KITE_FOUND False )
    return()
  endif()
else()
  message( STATUS "KITE_HOME set upstream: ${KITE_HOME}" )
endif()

# --- Find internal KITE Tools

# NOTE ALEX: Should check that files exists at least ...
if( WIN32 )
  set( KITE_PATH   "${KITE_HOME}\\scripts\\windows\\path" )
  set( KITE_R_CMD  "${KITE_PATH}\\r.bat" )
  set( KITE_C_CMD  "${KITE_PATH}\\c.bat" )
elseif( UNIX AND NOT APPLE )
  # TODO
  message( STATUS "FindKITE Module does not support Linux yet." )
elseif( APPLE )
  set( KITE_PATH   "${KITE_HOME}/scripts/mac/path" )
  set( KITE_R_CMD  "${KITE_PATH}/r-mac" )
  set( KITE_C_CMD  "${KITE_PATH}/c" )
endif()

#-------------------------------------------------------------------------------
# --- Find Components

find_program( __my_appium
  NAMES
    Appium
    appium
  )
if( NOT __my_appium )
  message( STATUS "Appium application was not found." )
else()
  if( APPLE )
    # if mac, also check for appium-for-mac
    # do we need something for ios ?
    find_program( __my_os_appium
      NAMES
        AppiumForMac
    )
  elseif( LINUX )
    # TODO
  elseif( WIN32 )
    find_program( __my_os_appium
      NAMES
        WinAppDriver.exe
      PATHS
        "C:/Program\ Files\ \(x86\)/Windows\ Application\ Driver"
      )
  endif()
  if( NOT __my_os_appium )
    set( KITE_Appium_FOUND 0 )
  else()
    set( KITE_Appium_FOUND 1 )
  endif()
endif()

#-------------------------------------------------------------------------------
# --- Prepare output

set( __component_failed False )
# handling components myself
foreach( component ${KITE_FIND_COMPONENTS} )
  message( STATUS "Requested KITE's ${component} component: searching..." )
  if( DEFINED KITE_${component}_FOUND )
    if( ${KITE_${component}_FOUND} )
      message( STATUS "Requested KITE's ${component}: FOUND." )
    else()
      message( STATUS "Requested KITE's ${component}: NOT FOUND." )
      set( __component_failed True )
    endif()
  else()
    message( STATUS "Requested KITE's ${component}: NOT DEFINED." )
    set( __component_failed True )
  endif()
endforeach()
if( __component_failed )
  set( KITE_FOUND False )
  return()
endif()

# handle the QUIETLY and REQUIRED arguments and set KITE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( KITE
  REQUIRED_VARS
    KITE_HOME
    KITE_PATH
    KITE_R_CMD
    KITE_C_CMD
  # HANDLE_COMPONENTS
  )
