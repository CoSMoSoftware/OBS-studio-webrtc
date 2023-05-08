# OBS CMake common version helper module

# cmake-format: off
# cmake-lint: disable=C0301
# cmake-format: on

# Attempt to automatically discover expected OBS version
if(NOT DEFINED OBS_VERSION_OVERRIDE)
  if(DEFINED RELEASE_CANDIDATE)
    if(RELEASE_CANDIDATE MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+-rc[0-9]+")
      string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)-rc([0-9])+" "\\1;\\2;\\3;\\4" _obs_release_candidate
                           ${RELEASE_CANDIDATE})
      list(GET _obs_release_candidate 0 1 2 _obs_version_canonical)
      set(_obs_version ${RELEASE_CANDIDATE})
    else()
      message(FATAL_ERROR "Invalid release candidate version supplied - must be <MAJOR>.<MINOR>.<PATCH>-rc<CANDIDATE>.")
    endif()
  elseif(DEFINED BETA)
    if(BETA MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+-beta[0-9]+")
      string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)-beta([0-9])+" "\\1;\\2;\\3;\\4" _obs_beta ${BETA})
      list(GET _obs_beta 0 1 2 _obs_version_canonical)
      set(_obs_version ${BETA})
    else()
      message(FATAL_ERROR "Invalid beta version supplied - must be <MAJOR>.<MINOR>.<PATCH>-beta<RELEASE>.")
    endif()
  elseif(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git describe --always --tags --dirty=-modified
      OUTPUT_VARIABLE _obs_version
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE _obs_version_result
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(_obs_version_result EQUAL 0)
      if(_obs_version MATCHES "rc[0-9]+$")
        set(RELEASE_CANDIDATE ${_obs_version})
      elseif(_obs_version MATCHES "beta[0-9]+$")
        set(BETA ${_obs_version})
      endif()

      string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1;\\2;\\3" _obs_version_canonical ${_obs_version})
    else()
      set(_obs_version ${_obs_default_version})
      set(_obs_version_canonical ${_obs_default_version})
    endif()
  endif()
else()
  if(OBS_VERSION_OVERRIDE MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+).*")
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1;\\2;\\3" _obs_version_canonical
                         ${OBS_VERSION_OVERRIDE})
    set(_obs_version ${OBS_VERSION_OVERRIDE})
  else()
    message(FATAL_ERROR "Invalid version supplied - must be <MAJOR>.<MINOR>.<PATCH>.")
  endif()
endif()

list(GET _obs_version_canonical 0 OBS_VERSION_MAJOR)
list(GET _obs_version_canonical 1 OBS_VERSION_MINOR)
list(GET _obs_version_canonical 2 OBS_VERSION_PATCH)
list(GET _obs_release_candidate 0 OBS_RELEASE_CANDIDATE_MAJOR)
list(GET _obs_release_candidate 1 OBS_RELEASE_CANDIDATE_MINOR)
list(GET _obs_release_candidate 2 OBS_RELEASE_CANDIDATE_PATCH)
list(GET _obs_release_candidate 3 OBS_RELEASE_CANDIDATE)
list(GET _obs_beta 0 OBS_BETA_MAJOR)
list(GET _obs_beta 1 OBS_BETA_MINOR)
list(GET _obs_beta 2 OBS_BETA_PATCH)
list(GET _obs_beta 3 OBS_BETA)

string(REPLACE ";" "." OBS_VERSION_CANONICAL "${_obs_version_canonical}")
string(REPLACE ";" "." OBS_VERSION "${_obs_version}")

if(OBS_RELEASE_CANDIDATE GREATER 0)
  message(
    AUTHOR_WARNING
      "******************************************************************************\n"
      "  + OBS-Studio - Release candidate detected, OBS_VERSION is now: ${OBS_VERSION}\n"
      "******************************************************************************")
elseif(OBS_BETA GREATER 0)
  message(
    AUTHOR_WARNING
      "******************************************************************************\n"
      "  + OBS-Studio - Beta detected, OBS_VERSION is now: ${OBS_VERSION}\n"
      "******************************************************************************")
endif()

unset(_obs_default_version)
unset(_obs_version)
unset(_obs_version_canonical)
unset(_obs_release_candidate)
unset(_obs_beta)
unset(_obs_version_result)
