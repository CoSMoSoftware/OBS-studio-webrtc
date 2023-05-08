add_executable(obs-x264-test)

target_sources(obs-x264-test PRIVATE obs-x264-test.c)
target_link_libraries(obs-x264-test PRIVATE OBS::opts-parser)

add_test(NAME obs-x264-test COMMAND obs-x264-test)

set_target_properties(obs-x264-test PROPERTIES FOLDER plugins/obs-x264)
