# Copyright Dr. Alex. Gouaillard (2015, 2020)

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
  message( STATUS "${OBS_WEBRTC_VENDOR_NAME}: Copying ./${_file} over to ${PROJECT_SOURCE_DIR}/${_subdir}." )
  get_filename_component( _subdir ${_file} DIRECTORY )
  if( NOT EXISTS  ${PROJECT_SOURCE_DIR}/${_file} )
    file( TOUCH
      ${PROJECT_SOURCE_DIR}/${_file}.old
      )
  else()
    file( RENAME
      ${PROJECT_SOURCE_DIR}/${_file}
      ${PROJECT_SOURCE_DIR}/${_file}.old
      )
  endif()
  file(
    COPY        ${_vendor_dir}/${_file}
    DESTINATION ${PROJECT_SOURCE_DIR}/${_subdir}
    )
endforeach()

