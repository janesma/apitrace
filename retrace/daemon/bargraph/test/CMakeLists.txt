find_package(Waffle REQUIRED)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon/bargraph
  )

set(BARGRAPH_TEST_SOURCE
  main_test.cpp
  test_bargraph.cpp
  test_bargraph_ctx.cpp
  test_bargraph_ctx.hpp
  )

add_executable(retrace_bargraph_test
  ${BARGRAPH_TEST_SOURCE}
  )

target_link_libraries(retrace_bargraph_test
  retrace_bargraph
  gtest
  waffle-1
  GL
  )

Lint( BARGRAPH_TEST_SOURCE )
