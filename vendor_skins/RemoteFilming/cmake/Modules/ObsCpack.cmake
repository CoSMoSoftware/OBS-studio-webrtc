
if(APPLE AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "Bundle")
elseif(WIN32 AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "WIX" "ZIP")
endif()

set(CPACK_PACKAGE_NAME "RemoteFilming")
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
		"rfs32" "Remote Filming (32bit)"
		"rfs64" "Remote Filming (64bit)")
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

	set(CPACK_PACKAGE_EXECUTABLES "obs${_output_suffix}" "Remote Filming")
	set(CPACK_CREATE_DESKTOP_LINKS "obs${_output_suffix}")
endif()

set(CPACK_BUNDLE_NAME "RemoteFilming")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/cmake/osxbundle/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obs.icns")
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obslaunch.sh")

set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/Modules/WIX.template.in")

if(INSTALLER_RUN)
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming")
	set(CPACK_WIX_UPGRADE_GUID "73525471-77bf-41c3-949c-5475e952ea70")
	set(CPACK_WIX_PRODUCT_GUID "ffc50d1c-f2c7-4af7-bd42-75c85c70bfc9")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-${OBS_VERSION}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Remote Filming (64bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming64")
	set(CPACK_WIX_UPGRADE_GUID "4223d0d0-a81a-486a-896c-6fce7e797be7")
	set(CPACK_WIX_PRODUCT_GUID "8b2bcf9f-8900-405a-8317-6e5bd19cbbee")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-x64-${OBS_VERSION}")
else()
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Remote Filming (32bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "RemoteFilming32")
	set(CPACK_WIX_UPGRADE_GUID "84e0e3f5-bd38-4d18-86a1-854e2528ef9e")
	set(CPACK_WIX_PRODUCT_GUID "1c4b137e-7e4c-4da4-aeef-a7430383eb09")
	set(CPACK_PACKAGE_FILE_NAME "remote-filming-x86-${OBS_VERSION}")
endif()

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

if(UNIX_STRUCTURE)
	set(CPACK_SET_DESTDIR TRUE)
endif()

include(CPack)
