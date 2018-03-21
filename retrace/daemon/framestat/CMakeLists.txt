# Copyright (c) 2018 Intel Corporation
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

cmake_minimum_required(VERSION 2.8)

include_directories (
  ../context
  ..
  )

set(CMAKE_INCLUDE_CURRENT_DIR ON)


set (FRAMESTAT_SRC
  glframe_loop.cpp
  glframe_loop.hpp
  ../glretrace_globals.cpp
  main.cpp
  )

add_executable(framestat
  ${FRAMESTAT_SRC}
)

if (WIN32)
  set (GL_LIB opengl32)
  set (THREAD_LIB Ws2_32)
else()
  set (GL_LIB GL)
  set (THREAD_LIB pthread dl)
endif()

target_link_libraries(framestat
  retrace_context
  retrace_gldispatch
  glretrace_common
  glhelpers
  glproc_gl
  ${GL_LIB}
  ${X11_X11_LIB}
  ${THREAD_LIB}
)

install (TARGETS framestat RUNTIME DESTINATION bin) 

if (NOT WIN32)
  Lint(FRAMESTAT_SRC)
endif()
