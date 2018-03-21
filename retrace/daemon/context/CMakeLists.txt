include(../Lint.cmake)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  )

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set (RETRACE_CONTEXT_SOURCES
  glframe_thread_context.cpp
  glframe_thread_context.hpp
  )  

add_library(retrace_context STATIC
  ${RETRACE_CONTEXT_SOURCES}
  )

target_link_libraries(retrace_context
  )

if (NOT WIN32)
  Lint( RETRACE_CONTEXT_SOURCES )
endif()

