

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/bargraph/test
  ${CMAKE_SOURCE_DIR}/thirdparty/md5
  )

if (NOT WIN32)
set (DAEMON_TEST_OS_SRC 
  retrace_metrics_test.cpp
  )
endif()

set (DAEMON_TEST_SRC
  ${DAEMON_TEST_OS_SRC}
  retrace_daemon_test.cpp
  retrace_file_transfer_test.cpp
  retrace_log_test.cpp
  retrace_socket_test.cpp
  retrace_thread_test.cpp
  main_test.cpp
  )

add_executable(retrace_daemon_test
  ${DAEMON_TEST_SRC}
  )

ADD_DEFINITIONS(-DCMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if (WIN32)
set (DAEMON_TEST_OS_LIBS
  opengl32
  Ws2_32
  )
else()
set (DAEMON_TEST_OS_LIBS
  retrace_ui_test_utilities
  waffle-1
  ${X11_X11_LIB}
  GL
  dl
  )
endif()

target_link_libraries(retrace_daemon_test
  retrace_daemon
  glretrace_common
  retrace_daemon
  retrace_gldispatch
  glhelpers
  glproc_gl
  libprotobuf
  gtest
  ${DAEMON_TEST_OS_LIBS}
  )

add_test (NAME daemon_test COMMAND $<TARGET_FILE:retrace_daemon_test>)

if (NOT WIN32)
  Lint(DAEMON_TEST_SRC)
endif()
