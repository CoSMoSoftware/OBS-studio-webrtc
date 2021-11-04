
if(APPLE AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "Bundle")
elseif(WIN32 AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "WIX" "ZIP")
endif()

set(CPACK_PACKAGE_NAME "Wowza")
set(CPACK_PACKAGE_VENDOR "wowza.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Wowza OBS - Real-Time - Live video and audio streaming and recording software")
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
		"obs32" "Wowza OBS - Real-Time (32bit)"
		"obs64" "Wowza OBS - Real-Time (64bit)")
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

	set(CPACK_PACKAGE_EXECUTABLES "obs${_output_suffix}" "Wowza OBS - Real-Time")
	set(CPACK_CREATE_DESKTOP_LINKS "obs${_output_suffix}")
endif()

set(CPACK_BUNDLE_NAME "Wowza OBS - Real-Time")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/cmake/osxbundle/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obs.icns")
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obslaunch.sh")

set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/Modules/WIX.template.in")

if(INSTALLER_RUN)
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Wowza OBS - Real-Time")
	set(CPACK_WIX_UPGRADE_GUID "710b0878-9744-4eff-b228-6a5deb9322c3")
	set(CPACK_WIX_PRODUCT_GUID "5f425f73-5f5e-48c3-8dc1-9bb6416c1180")
	set(CPACK_PACKAGE_FILE_NAME "wowza-obs-${OBS_VERSION}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Wowza OBS - Real-Time (64bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Wowza64")
	set(CPACK_WIX_UPGRADE_GUID "8d61c270-0a68-42b9-86d2-370f493920f0")
	set(CPACK_WIX_PRODUCT_GUID "f7cfa15e-8553-4670-a331-43da88d5f256")
	set(CPACK_PACKAGE_FILE_NAME "wowza-obs-x64-${OBS_VERSION}")
else()
	if(WIN32)
		set(CPACK_PACKAGE_NAME "Wowza OBS - Real-Time(32bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "Wowza32")
	set(CPACK_WIX_UPGRADE_GUID "31849e16-d9f9-4d36-a641-b170c83b03f9")
	set(CPACK_WIX_PRODUCT_GUID "a627b263-bd6f-4f40-9e32-66e21fb02486")
	set(CPACK_PACKAGE_FILE_NAME "wowza-obs-x86-${OBS_VERSION}")
endif()

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

if(UNIX_STRUCTURE)
	set(CPACK_SET_DESTDIR TRUE)
endif()

include(CPack)
