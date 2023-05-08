
if(APPLE AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "Bundle")
elseif(WIN32 AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "NSIS" "ZIP")
endif()

set(CPACK_PACKAGE_NAME "OBS-WebRTC")
set(CPACK_PACKAGE_VENDOR "obsproject.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OBS-WebRTC - Live video and audio streaming and recording software")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/UI/data/license/gplv2.txt")

set(CPACK_PACKAGE_VERSION_MAJOR "29")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
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
		"obs32" "OBS-WebRTC (32bit)"
		"obs64" "OBS-WebRTC (64bit)")
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

	set(CPACK_PACKAGE_EXECUTABLES "obs${_output_suffix}" "OBS-WebRTC")
	set(CPACK_CREATE_DESKTOP_LINKS "obs${_output_suffix}")
	# Set "start in" path for OBS-WebRTC shortcut
	set(CPACK_NSIS_CREATE_ICONS "SetOutPath '$INSTDIR\\\\bin\\\\${_output_suffix}bit'")
	set(CPACK_NSIS_CREATE_ICONS_EXTRA
		"CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\OBS-WebRTC.lnk' '$INSTDIR\\\\bin\\\\${_output_suffix}bit\\\\obs${_output_suffix}.exe'")
	set(CPACK_NSIS_DELETE_ICONS_EXTRA
		"Delete '$SMPROGRAMS\\\\$START_MENU\\\\Uninstall.lnk'")
endif()

set(CPACK_BUNDLE_NAME "OBS-WebRTC")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/cmake/osxbundle/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obs.icns")
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obslaunch.sh")

set(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/cmake/Modules/WIX.template.in")

set(CPACK_NSIS_MUI_ICON     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bundle/windows/obs-studio.ico")
set(CPACK_NSIS_MUI_UNIICON  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bundle/windows/obs-studio.ico")
set(CPACK_NSIS_PACKAGE_NAME "OBS-WebRTC ${OBS_VERSION}")
set(CPACK_NSIS_DISPLAY_NAME "OBS-WebRTC ${OBS_VERSION}")
set(CPACK_NSIS_MODIFY_PATH  ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

if(INSTALLER_RUN)
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "OBSWebRTC")
	set(CPACK_WIX_UPGRADE_GUID "1f59ff79-2a3c-43c1-b2b2-033a5e6342eb")
	set(CPACK_WIX_PRODUCT_GUID "0c7bec2a-4f07-41b2-9dff-d64b09c9c384")
	set(CPACK_PACKAGE_FILE_NAME "obs-webrtc-${OBS_VERSION}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		set(CPACK_PACKAGE_NAME "OBS-WebRTC (64bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "OBSWebRTC64")
	set(CPACK_WIX_UPGRADE_GUID "44c72510-2e8e-489c-8bc0-2011a9631b0b")
	set(CPACK_WIX_PRODUCT_GUID "ca5bf4fe-7b38-4003-9455-de249d03caac")
	set(CPACK_PACKAGE_FILE_NAME "obs-webrtc-x64-${OBS_VERSION}")
else()
	if(WIN32)
		set(CPACK_PACKAGE_NAME "OBS-WebRTC (32bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "OBSWebRTC32")
	set(CPACK_WIX_UPGRADE_GUID "a26acea4-6190-4470-9fb9-f6d32f3ba030")
	set(CPACK_WIX_PRODUCT_GUID "8e24982d-b0ab-4f66-9c90-f726f3b64682")
	set(CPACK_PACKAGE_FILE_NAME "obs-webrtc-x86-${OBS_VERSION}")
endif()

set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

if(UNIX AND NOT APPLE)
	cmake_host_system_information(RESULT PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
#	if(${DISTRO_VERSION_ID} VERSION_GREATER_EQUAL "22.04")
		set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-wayland")
#	endif()
endif()

if(UNIX_STRUCTURE)
	set(CPACK_SET_DESTDIR TRUE)
endif()

include(CPack)
