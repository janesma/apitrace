

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/bargraph/test
  )

set (DAEMON_TEST_SRC
  retrace_daemon_test.cpp
  retrace_log_test.cpp
  retrace_metrics_test.cpp
  retrace_thread_test.cpp
  main_test.cpp
  )

add_executable(retrace_daemon_test
  ${DAEMON_TEST_SRC}
  )

ADD_DEFINITIONS(-DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(retrace_daemon_test
  retrace_ui_test_utilities
  retrace_daemon
  glretrace_common
  retrace_daemon
  retrace_gldispatch
  glhelpers
  glproc_gl
  gtest
  waffle-1
  GL
  ${X11_X11_LIB}
  dl
  )

add_test (NAME daemon_test COMMAND $<TARGET_FILE:retrace_daemon_test>)

if (NOT WIN32)
  Lint(DAEMON_TEST_SRC)
endif()
