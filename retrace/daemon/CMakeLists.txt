find_package(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders
  playback.proto
  )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGOOGLE_PROTOBUF_NO_RTTI")

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace
  ${CMAKE_SOURCE_DIR}/lib/image
  ${CMAKE_BINARY_DIR}/dispatch
  ${CMAKE_SOURCE_DIR}/dispatch
  ${CMAKE_SOURCE_DIR}/common
  ${CMAKE_SOURCE_DIR}/helpers
  )

add_library(retrace_daemon STATIC
  ${ProtoHeaders}
  ${ProtoSources}
  glframe_retrace.hpp
  glframe_retrace.cpp
  glframe_state.hpp
  glframe_state.cpp
  glretrace_globals.cpp
  glframe_socket.hpp
  glframe_socket.cpp
  glframe_traits.hpp
  )

target_link_libraries(retrace_daemon
  glretrace_common
  )

add_subdirectory (test)
add_subdirectory (ui)
