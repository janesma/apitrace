cmake_minimum_required(VERSION 2.8)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  )

set(CMAKE_INCLUDE_CURRENT_DIR ON)


set (SERVER_SRC
  main.cpp
  )

add_executable(frameretrace_server
  ${SERVER_SRC}
)

if (WIN32)
  set (GL_LIB opengl32)
  set (THREAD_LIB Ws2_32)
else()
  set (GL_LIB GL)
  set (THREAD_LIB pthread dl)
endif()

target_link_libraries(frameretrace_server
  retrace_daemon
  glretrace_common
  glhelpers
  glproc_gl
  libprotobuf
  ${GL_LIB}
  ${X11_X11_LIB}
  ${THREAD_LIB}
)

install (TARGETS frameretrace_server RUNTIME DESTINATION bin) 

if (NOT WIN32)
  Lint(SERVER_SRC)
endif()
