include(Lint.cmake)

include(FindProtobuf)

if (WIN32)
set (Protobuf_PROTOC_EXECUTABLE protoc)
else()
set (Protobuf_PROTOC_EXECUTABLE ${CMAKE_BINARY_DIR}/protoc)
set (PROTOBUF_PROTOC_EXECUTABLE ${CMAKE_BINARY_DIR}/protoc)
endif()

PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders
  playback.proto
  )

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace
  ${CMAKE_SOURCE_DIR}/lib/image
  ${CMAKE_BINARY_DIR}/dispatch
  ${CMAKE_SOURCE_DIR}/dispatch
  ${CMAKE_SOURCE_DIR}/common
  ${CMAKE_SOURCE_DIR}/helpers
  ${CMAKE_BINARY_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/gldispatch
  ${CMAKE_SOURCE_DIR}/thirdparty/md5
  )

set (RETRACE_LINUX_SOURCE
  glframe_os_linux.cpp
  glframe_stderr.cpp
  glframe_thread_linux.cpp
  glframe_gpu_speed_linux.cpp
  )

set (RETRACE_WIN_SOURCE
  glframe_os_win.cpp
  glframe_thread_win.cpp
  glframe_win_shaders.cpp
  )

if (WIN32)
  set (RETRACE_OS_SOURCE ${RETRACE_WIN_SOURCE})
else()
  set (RETRACE_OS_SOURCE ${RETRACE_LINUX_SOURCE})
endif()

set (RETRACE_SOURCES
  ${RETRACE_OS_SOURCE}
  glframe_gpu_speed.hpp
  glframe_metrics.cpp
  glframe_metrics.hpp
  glframe_os.hpp
  glframe_logger.cpp
  glframe_logger.hpp
  glframe_retrace.cpp
  glframe_retrace.hpp
  glframe_retrace_interface.hpp
  glframe_retrace_render.cpp
  glframe_retrace_render.hpp
  glframe_retrace_skeleton.cpp
  glframe_retrace_skeleton.hpp
  glframe_retrace_stub.cpp
  glframe_retrace_stub.hpp
  glframe_socket.hpp
  glframe_socket.cpp
  glframe_state.cpp
  glframe_state.hpp
  glframe_stderr.hpp
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
  libprotobuf
  )

add_dependencies(retrace_daemon protoc)

if (NOT WIN32)
  Lint( RETRACE_SOURCES )
endif()

add_subdirectory (framestat)
add_subdirectory (test)
add_subdirectory (ui)
add_subdirectory (server)
add_subdirectory (bargraph)
add_subdirectory (gldispatch)
