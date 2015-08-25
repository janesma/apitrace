include(../Lint.cmake)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  )

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Widgets REQUIRED) 

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#add_definitions(${Qt5Widgets_DEFINITIONS})

#qt5_add_resources(RESOURCE_ADDED resources.qrc)

set (BARGRAPH_SOURCES
  glframe_bargraph.cpp
  glframe_bargraph.hpp
  glframe_bargraphmodel.cpp
  glframe_bargraphmodel.hpp
  glframe_glhelper.cpp
  glframe_glhelper.hpp
  glframe_qbargraph.cpp
  glframe_qbargraph.hpp
  )

Lint( BARGRAPH_SOURCES )

add_library(retrace_bargraph STATIC
  ${BARGRAPH_SOURCES}
  )

qt5_use_modules(retrace_bargraph Core Gui OpenGL Quick) 

add_subdirectory(test)
