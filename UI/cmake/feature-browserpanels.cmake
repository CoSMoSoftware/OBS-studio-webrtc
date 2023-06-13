if(TARGET OBS::browser-panels)
  target_enable_feature(obs-webrtc "Browser panels" BROWSER_AVAILABLE)

  target_link_libraries(obs-webrtc PRIVATE OBS::browser-panels)

  target_sources(obs-webrtc PRIVATE window-dock-browser.cpp window-dock-browser.hpp window-extra-browsers.cpp
                                    window-extra-browsers.hpp)
endif()
