cmake_minimum_required(VERSION 2.8)

include_directories (
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
