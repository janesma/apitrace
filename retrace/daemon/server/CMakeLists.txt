cmake_minimum_required(VERSION 2.8)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  )

set(CMAKE_INCLUDE_CURRENT_DIR ON)


set (SERVER_SRC
  main.cpp
  )

add_executable(frame_retrace_server
  ${SERVER_SRC}
)

target_link_libraries(frame_retrace_server
  retrace_daemon
  glretrace_common
  glhelpers
  glproc_gl
  protobuf
  GL
  ${X11_X11_LIB}
  pthread
  dl
)

if (NOT WIN32)
  Lint(SERVER_SRC)
endif()
