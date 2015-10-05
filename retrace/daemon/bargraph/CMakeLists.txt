# Copyright (c) 2015 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
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
  glframe_glhelper.cpp
  glframe_glhelper.hpp
  glframe_qbargraph.cpp
  glframe_qbargraph.hpp
  glframe_qselection.cpp
  glframe_qselection.hpp
  )

Lint( BARGRAPH_SOURCES )

add_library(retrace_bargraph STATIC
  ${BARGRAPH_SOURCES}
  )

qt5_use_modules(retrace_bargraph Core Gui OpenGL Quick) 

add_subdirectory(test)
add_subdirectory(ui)
