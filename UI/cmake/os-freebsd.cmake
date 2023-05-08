target_sources(obs PRIVATE platform-x11.cpp)
target_compile_definitions(obs PRIVATE OBS_INSTALL_PREFIX="${OBS_INSTALL_PREFIX}")
target_link_libraries(obs PRIVATE Qt::GuiPrivate procstat)

if(TARGET OBS::python)
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  target_link_libraries(obs PRIVATE Python::Python)
  target_link_options(obs PRIVATE LINKER:-no-as-needed)
endif()
