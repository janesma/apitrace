include(Lint.cmake)

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
  ${CMAKE_BINARY_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/gldispatch
  )

set (RETRACE_LINUX_SOURCE glframe_os_linux.cpp)

set (RETRACE_WIN_SOURCE glframe_os_win.cpp)

if (WIN32)
  set (RETRACE_OS_SOURCE ${RETRACE_WIN_SOURCE})
else()
  set (RETRACE_OS_SOURCE ${RETRACE_LINUX_SOURCE})
endif()

set (RETRACE_SOURCES
  ${RETRACE_OS_SOURCE}
  glframe_metrics.cpp
  glframe_metrics.hpp
  glframe_os.hpp
  glframe_logger.cpp
  glframe_logger.hpp
  glframe_retrace.cpp
  glframe_retrace.hpp
  glframe_retrace_render.cpp
  glframe_retrace_render.hpp
  glframe_retrace_skeleton.cpp
  glframe_retrace_skeleton.hpp
  glframe_retrace_stub.cpp
  glframe_retrace_stub.hpp
  glframe_socket.cpp
  glframe_socket.hpp
  glframe_state.cpp
  glframe_state.hpp
  glframe_thread.cpp
  glframe_thread.hpp
  glframe_traits.hpp
  glretrace_globals.cpp
)  

add_library(retrace_daemon STATIC
  ${ProtoHeaders}
  ${ProtoSources}
  ${RETRACE_SOURCES}
  )

target_link_libraries(retrace_daemon
  glretrace_common
  retrace_gldispatch
  )

Lint( RETRACE_SOURCES )

add_subdirectory (test)
add_subdirectory (ui)
add_subdirectory (server)
add_subdirectory (bargraph)
add_subdirectory (gldispatch)
