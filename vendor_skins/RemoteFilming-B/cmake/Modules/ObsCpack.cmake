
if(APPLE AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "Bundle")
elseif(WIN32 AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "WIX" "ZIP")
endif()

set(CPACK_PACKAGE_NAME "B-CAM")
set(CPACK_PACKAGE_VENDOR "remotefilming.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Remote Filming - Live video and audio streaming and recording software")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/UI/data/license/gplv2.txt")

set(CPACK_PACKAGE_VERSION_MAJOR "28")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "2")
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
		"obs32" "Remote Filming-B (32bit)"
		"obs64" "Remote Filming-B (64bit)")
	set(CPACK_CREATE_DESKTOP_LINKS
		"obs32"
		"obs64")
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

	set(CPACK_PACKAGE_EXECUTABLES "obs${_output_suffix}" "Remote Filming-B")
	set(CPACK_CREATE_DESKTOP_LINKS "obs${_output_suffix}")
endif()

set(CPACK_BUNDLE_NAME "RemoteFilming-B")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/cmake/osxbundle/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obs.icns")
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obslaunch.sh")

set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/Modules/WIX.template.in")

if(INSTALLER_RUN)
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-B")
	set(CPACK_WIX_UPGRADE_GUID "8ac0c9e4-1c67-4a0c-b583-54670a066010")
	set(CPACK_WIX_PRODUCT_GUID "7348e857-7954-4566-a319-248b05c0b2b1")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-B-${OBS_VERSION}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Remote Filming-B (64bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-B64")
	set(CPACK_WIX_UPGRADE_GUID "30e68ff9-a7e6-402d-bff5-9a5a2478f2d2")
	set(CPACK_WIX_PRODUCT_GUID "17624f6a-85c1-4874-83f5-434c4415042f")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-B-x64-${OBS_VERSION}")
else()
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Remote Filming-B (32bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-B32")
	set(CPACK_WIX_UPGRADE_GUID "733ab004-c200-466c-b99d-b474600e3bfe")
	set(CPACK_WIX_PRODUCT_GUID "d79f137a-f43b-4b8e-8465-1933b29fff87")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-B-x86-${OBS_VERSION}")
endif()

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

if(UNIX_STRUCTURE)
	set(CPACK_SET_DESTDIR TRUE)
endif()

include(CPack)
