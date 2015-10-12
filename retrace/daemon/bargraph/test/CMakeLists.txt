find_package(Waffle REQUIRED)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon/bargraph
  )

find_package(Qt5Core REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(BARGRAPH_TEST_SOURCE
  main_test.cpp
  test_bargraph.cpp
  test_bargraph_ctx.cpp
  test_bargraph_ctx.hpp
  test_selection.cpp
  test_selection.hpp
  )

add_executable(retrace_bargraph_test
  ${BARGRAPH_TEST_SOURCE}
  )

target_link_libraries(retrace_bargraph_test
  retrace_bargraph
  retrace_gldispatch
  gtest
  waffle-1
  GL
  dl
  )

qt5_use_modules(retrace_bargraph_test Core) 

add_test (NAME bargraph_test COMMAND $<TARGET_FILE:retrace_bargraph_test>)

Lint( BARGRAPH_TEST_SOURCE )
