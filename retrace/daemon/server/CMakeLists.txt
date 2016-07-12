cmake_minimum_required(VERSION 2.8)

include(FindProtobuf)
find_package(Protobuf REQUIRED)

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

if (WIN32)
  set (GL_LIB opengl32)
  set (THREAD_LIB "")
else()
  set (GL_LIB GL)
  set (THREAD_LIB pthread dl)
endif()

MESSAGE (STATUS "PROTOBUF_LIBRARY: " ${PROTOBUF_LIBRARY})
target_link_libraries(frame_retrace_server
  retrace_daemon
  glretrace_common
  glhelpers
  glproc_gl
  ${PROTOBUF_LIBRARY}
  ${GL_LIB}
  ${X11_X11_LIB}
  ${THREAD_LIB}
)

if (NOT WIN32)
  Lint(SERVER_SRC)
endif()
