if(RESTREAM_CLIENTID
   AND RESTREAM_HASH
   AND TARGET OBS::browser-panels)
  target_sources(obs PRIVATE auth-restream.cpp auth-restream.hpp)
  target_enable_feature(obs "Restream API connection" RESTREAM_ENABLED)
else()
  target_disable_feature(obs "Restream API connection")
  set(RESTREAM_CLIENTID "")
  set(RESTREAM_HASH "0")
endif()
