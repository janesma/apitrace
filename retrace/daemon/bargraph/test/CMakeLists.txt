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
  test_selection.cpp
  test_selection.hpp
  )

set(TEST_UTILITIES_SOURCE
  test_bargraph_ctx.cpp
  test_bargraph_ctx.hpp
)  

add_executable(retrace_bargraph_test
  ${BARGRAPH_TEST_SOURCE}
  )

add_library(retrace_ui_test_utilities
  ${TEST_UTILITIES_SOURCE}
  )


target_link_libraries(retrace_bargraph_test
  retrace_ui_test_utilities
  retrace_bargraph
  retrace_gldispatch
  libprotobuf
  gtest
  waffle-1
  Qt5::Core
  GL
  dl
  )

add_test (NAME bargraph_test COMMAND $<TARGET_FILE:retrace_bargraph_test>)

set(LINT_SOURCE
  ${BARGRAPH_TEST_SOURCE}
  ${TEST_UTILITIES_SOURCE}
  )

if (NOT WIN32)
  Lint( LINT_SOURCE )
endif()
