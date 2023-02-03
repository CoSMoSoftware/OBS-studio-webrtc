
if(APPLE AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "Bundle")
elseif(WIN32 AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "WIX" "ZIP")
endif()

set(CPACK_PACKAGE_NAME "C-CAM")
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
		"rfs32" "Remote Filming-C (32bit)"
		"rfs64" "Remote Filming-C (64bit)")
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

	set(CPACK_PACKAGE_EXECUTABLES "obs${_output_suffix}" "Remote Filming-C")
	set(CPACK_CREATE_DESKTOP_LINKS "obs${_output_suffix}")
endif()

set(CPACK_BUNDLE_NAME "RemoteFilming-C")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/cmake/osxbundle/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obs.icns")
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obslaunch.sh")

set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/Modules/WIX.template.in")

if(INSTALLER_RUN)
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-C")
	set(CPACK_WIX_UPGRADE_GUID "469a36d2-eeb8-49e2-8027-59d1282a71c6")
	set(CPACK_WIX_PRODUCT_GUID "22515b73-860b-491c-9bed-d0f221434c06")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-C-${OBS_VERSION}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Remote Filming-C (64bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-C64")
	set(CPACK_WIX_UPGRADE_GUID "f6db69f0-67c6-484f-9b27-6a36b7403d4e")
	set(CPACK_WIX_PRODUCT_GUID "0f7316b8-0870-44c3-90a3-c54cbd967dc5")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-C-x64-${OBS_VERSION}")
else()
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Remote Filming-C (32bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming-C32")
	set(CPACK_WIX_UPGRADE_GUID "f2b47d30-49c4-4817-9c96-695aacf72291")
	set(CPACK_WIX_PRODUCT_GUID "bb972f98-4670-4c00-b594-4f6cfbaef2cb")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-C-x86-${OBS_VERSION}")
endif()

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

if(UNIX_STRUCTURE)
	set(CPACK_SET_DESTDIR TRUE)
endif()

include(CPack)
