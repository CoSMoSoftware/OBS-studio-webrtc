
if(BUILD_TESTS)
	find_package( KITE COMPONENTS Appium )
	if( NOT KITE_FOUND )
  		message( STATUS "Could not find  KITE. Interop tests will be skipped." )
	else()
		add_subdirectory(KITE)
	endif()

	add_subdirectory(test-input)

	if(WIN32)
		add_subdirectory(win)
	endif()

	if(APPLE AND UNIX)
		add_subdirectory(osx)
	endif()
endif()

if (ENABLE_UNIT_TESTS)
	add_subdirectory(cmocka)
endif()
