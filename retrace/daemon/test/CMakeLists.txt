

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  )

add_executable(retrace_daemon_test
  retrace_daemon_test.cpp
  main_test.cpp
  )

ADD_DEFINITIONS(-DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

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

add_test (NAME daemon_test COMMAND $<TARGET_FILE:retrace_daemon_test>)
