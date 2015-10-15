cmake_minimum_required(VERSION 2.8)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/bargraph
  )

find_package(Qt5Widgets REQUIRED) 
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
#find_package(Qt5OpenGL REQUIRED)


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(${Qt5Widgets_DEFINITIONS})

qt5_add_resources(RESOURCE_ADDED resources.qrc)

set (UI_SRC
  main.cpp
  glframe_retrace_images.hpp
  glframe_retrace_images.cpp
  glframe_retrace_model.hpp
  glframe_retrace_model.cpp
  )
add_executable(frame_retrace
  ${UI_SRC}
  ${UIS_HDRS}
  ${RESOURCE_ADDED} 
)

qt5_use_modules(frame_retrace Core Gui Concurrent Quick) 
# OpenGL
#Widgets

target_link_libraries(frame_retrace
  retrace_daemon
  retrace_bargraph
  glretrace_common
  glhelpers
  glproc_gl
  protobuf
  GL
  ${X11_X11_LIB}
  pthread
  dl
)

Lint(UI_SRC)
