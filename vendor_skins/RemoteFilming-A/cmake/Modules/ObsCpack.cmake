
if(APPLE AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "Bundle")
elseif(WIN32 AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "WIX" "ZIP")
endif()

set(CPACK_PACKAGE_NAME "A-CAM")
set(CPACK_PACKAGE_VENDOR "remotefilming.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Remote Filming - Live video and audio streaming and recording software")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/UI/data/license/gplv2.txt")

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

if(NOT DEFINED OBS_VERSION_OVERRIDE)
	if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
		execute_process(COMMAND git describe --always --tags --dirty=-modified
			OUTPUT_VARIABLE OBS_VERSION
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	else()
		set(OBS_VERSION "${CPACK_PACKAGE_VERSION}")
	endif()
else()
	set(OBS_VERSION "${OBS_VERSION_OVERRIDE}")
endif()

if("${OBS_VERSION}" STREQUAL "")
	message(FATAL_ERROR "Failed to configure OBS_VERSION. Either set OBS_VERSION_OVERRIDE or ensure `git describe` succeeds.")
endif()
MESSAGE(STATUS "OBS_VERSION: ${OBS_VERSION}")

if(INSTALLER_RUN)
	set(CPACK_PACKAGE_EXECUTABLES
		"rfs32" "Remote Filming-A (32bit)"
		"rfs64" "Remote Filming-A (64bit)")
	set(CPACK_CREATE_DESKTOP_LINKS
		"rfs32"
		"rfs64")
else()
	if(WIN32)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set(_output_suffix "64")
		else()
			set(_output_suffix "32")
		endif()
	else()
		set(_output_suffix "")
	endif()

	set(CPACK_PACKAGE_EXECUTABLES "rfs${_output_suffix}" "Remote Filming-A")
	set(CPACK_CREATE_DESKTOP_LINKS "rfs${_output_suffix}")
endif()

set(CPACK_BUNDLE_NAME "RemoteFilming-A")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/cmake/osxbundle/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obs.icns")
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obslaunch.sh")

set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/Modules/WIX.template.in")

if(INSTALLER_RUN)
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-A")
	set(CPACK_WIX_UPGRADE_GUID "62572107-987e-4ae0-9e8d-99923b2a89fd")
	set(CPACK_WIX_PRODUCT_GUID "0652c192-f8e0-42af-b288-2afdc19670d3")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-A-${OBS_VERSION}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Remote Filming-A (64bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-A64")
	set(CPACK_WIX_UPGRADE_GUID "9e459891-ab0d-446a-91b1-1c086c7bb980")
	set(CPACK_WIX_PRODUCT_GUID "198f8505-c56b-441c-a51f-4fd29e6e6f06")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-A-x64-${OBS_VERSION}")
else()
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Remote Filming-A (32bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-A32")
	set(CPACK_WIX_UPGRADE_GUID "20296c2e-a820-4d34-b7cd-efd970b19474")
	set(CPACK_WIX_PRODUCT_GUID "27958658-472f-4c7a-97d9-7072fa43bbf7")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-A-x86-${OBS_VERSION}")
endif()

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

if(UNIX_STRUCTURE)
	set(CPACK_SET_DESTDIR TRUE)
endif()

include(CPack)
