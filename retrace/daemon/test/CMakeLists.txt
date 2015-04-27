

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  )

add_executable(retrace_daemon_test
  retrace_daemon_test.cpp
  main_test.cpp
  )

target_link_libraries(retrace_daemon_test
  retrace_daemon
  glretrace_common
  retrace_daemon
  glhelpers
  glproc_gl
  gtest
  GL
  ${X11_X11_LIB}
  dl
  )
