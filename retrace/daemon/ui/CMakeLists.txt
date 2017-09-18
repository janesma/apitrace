cmake_minimum_required(VERSION 2.8)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/bargraph
  )

find_package(Qt5Widgets REQUIRED) 
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Concurrent REQUIRED)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(${Qt5Widgets_DEFINITIONS})

qt5_add_resources(RESOURCE_ADDED resources.qrc)

set (UI_SRC
  main.cpp
  glframe_api_model.hpp
  glframe_api_model.cpp
  glframe_batch_model.hpp
  glframe_batch_model.cpp
  glframe_experiment_model.cpp
  glframe_experiment_model.hpp
  glframe_metrics_model.hpp
  glframe_metrics_model.cpp
  glframe_qutil.hpp
  glframe_qutil.cpp
  glframe_rendertarget_model.cpp
  glframe_rendertarget_model.hpp
  glframe_retrace_images.hpp
  glframe_retrace_images.cpp
  glframe_retrace_model.hpp
  glframe_retrace_model.cpp
  glframe_shader_model.hpp
  glframe_shader_model.cpp
  glframe_uniform_model.hpp
  glframe_uniform_model.cpp
  )

add_executable(frameretrace
  ${UI_SRC}
  ${UIS_HDRS}
  ${RESOURCE_ADDED} 
)

if (WIN32)
  set (GL_LIB opengl32)
  set (THREAD_LIB Ws2_32)
else()
  set (GL_LIB GL)
  set (THREAD_LIB pthread dl)
endif()

target_link_libraries(frameretrace
  retrace_daemon
  retrace_bargraph
  glretrace_common
  glhelpers
  glproc_gl
  Qt5::Core Qt5::Gui Qt5::Concurrent Qt5::Quick Qt5::Widgets
  libprotobuf
  ${GL_LIB}
  ${X11_X11_LIB}
  ${THREAD_LIB}
)

install (TARGETS frameretrace RUNTIME DESTINATION bin) 

Lint(UI_SRC)
