include_directories (
  ${CMAKE_SOURCE_DIR}/retrace
  ${CMAKE_SOURCE_DIR}/lib/image
  ${CMAKE_BINARY_DIR}/dispatch
  ${CMAKE_SOURCE_DIR}/dispatch
  ${CMAKE_SOURCE_DIR}/common
  ${CMAKE_SOURCE_DIR}/helpers
  )

add_library(retrace_daemon STATIC
  glframe_retrace.hpp
  glframe_retrace.cpp
  glframe_state.hpp
  glframe_state.cpp
  glretrace_globals.cpp
  )

target_link_libraries(retrace_daemon
  glretrace_common
  )

add_subdirectory (test)
