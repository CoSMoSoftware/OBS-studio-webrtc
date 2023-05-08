if(TARGET OBS::browser-panels)
  target_enable_feature(obs "Browser panels" BROWSER_AVAILABLE)

  target_link_libraries(obs PRIVATE OBS::browser-panels)

  target_sources(obs PRIVATE window-dock-browser.cpp window-dock-browser.hpp window-extra-browsers.cpp
                                    window-extra-browsers.hpp)
endif()
