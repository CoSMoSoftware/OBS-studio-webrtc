if( NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/${OBS_WEBRTC_VENDOR_NAME} )
  message( WARNING "You asked for ${OBS_WEBRTC_VENDOR_NAME}'s skin, but no such vendor files could be found." )
  return()
endif()

file(
  GLOB_RECURSE vendor_files_to_copy_over
  RELATIVE ${CMAKE_CURRENT_LIST_DIR}/${OBS_WEBRTC_VENDOR_NAME}
  ${OBS_WEBRTC_VENDOR_NAME}/*
  )

# NOTE ALEX: TODO: implement real copy
foreach( _file ${vendor_files_to_copy_over} )
  message( STATUS "Vendor file to copy over: ${_file}." )
endforeach()

