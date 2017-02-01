cmake_minimum_required(VERSION 2.8)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/bargraph
  )

find_package(Qt5Widgets REQUIRED) 
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(${Qt5Widgets_DEFINITIONS})

qt5_add_resources(RESOURCE_ADDED resources.qrc)

set (UI_SRC
  main.cpp
  glframe_api_model.hpp
  glframe_api_model.cpp
  glframe_metrics_model.hpp
  glframe_metrics_model.cpp
  glframe_qutil.hpp
  glframe_qutil.cpp
  glframe_retrace_images.hpp
  glframe_retrace_images.cpp
  glframe_retrace_model.hpp
  glframe_retrace_model.cpp
  glframe_shader_model.hpp
  glframe_shader_model.cpp
  )

add_executable(frame_retrace
  ${UI_SRC}
  ${UIS_HDRS}
  ${RESOURCE_ADDED} 
)

qt5_use_modules(frame_retrace Core Gui Concurrent Quick) 

if (WIN32)
  set (GL_LIB opengl32)
  set (THREAD_LIB Ws2_32)
else()
  set (GL_LIB GL)
  set (THREAD_LIB pthread dl)
endif()

target_link_libraries(frame_retrace
  retrace_daemon
  retrace_bargraph
  glretrace_common
  glhelpers
  glproc_gl
  libprotobuf
  ${GL_LIB}
  ${X11_X11_LIB}
  ${THREAD_LIB}
)

Lint(UI_SRC)
