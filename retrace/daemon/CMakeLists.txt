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
  glframe_retrace.cpp
  glframe_retrace.hpp
  glframe_socket.cpp
  glframe_socket.hpp
  glframe_state.cpp
  glframe_state.hpp
  glframe_thread.cpp
  glframe_thread.hpp
  glframe_os.hpp
  glframe_traits.hpp
  glretrace_globals.cpp
  glframe_retrace_stub.hpp
  glframe_retrace_stub.cpp
  glframe_retrace_skeleton.hpp
  glframe_retrace_skeleton.cpp
)  

macro (FullPath out_var _IN_FILES)
  foreach( _in_file ${${_IN_FILES}})
    list (APPEND out_list ${CMAKE_CURRENT_SOURCE_DIR}/${_in_file})
  endforeach(_in_file)
  set (${out_var} ${out_list})
endmacro(FullPath)

FullPath(src_full_paths RETRACE_SOURCES)

add_library(retrace_daemon STATIC
  ${ProtoHeaders}
  ${ProtoSources}
  ${RETRACE_SOURCES}
  )

target_link_libraries(retrace_daemon
  glretrace_common
  )

add_custom_target(lint ALL python ${CMAKE_CURRENT_SOURCE_DIR}/cpplint.py --output=emacs ${src_full_paths})

add_subdirectory (test)
add_subdirectory (ui)
