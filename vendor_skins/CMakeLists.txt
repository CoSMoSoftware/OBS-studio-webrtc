set( _vendor_dir ${CMAKE_CURRENT_LIST_DIR}/${OBS_WEBRTC_VENDOR_NAME} )

if( NOT EXISTS ${_vendor_dir} )
  message( WARNING "You asked for ${OBS_WEBRTC_VENDOR_NAME}'s skin, but no such vendor files could be found." )
  return()
endif()

file(
  GLOB_RECURSE _vendor_files_to_copy_over
  RELATIVE ${_vendor_dir}
  ${OBS_WEBRTC_VENDOR_NAME}/*
  )

foreach( _file ${_vendor_files_to_copy_over} )
  get_filename_component( _vendor_subdir ${_file} DIRECTORY )
  message( STATUS "${OBS_WEBRTC_VENDOR_NAME}: Copying ./${_file} over to ${PROJECT_SOURCE_DIR}/${_vendor_subdir}." )
  file(
    COPY        ${_vendor_dir}/${_file}
    DESTINATION ${PROJECT_SOURCE_DIR}/${_vendor_subdir}
    )
endforeach()

