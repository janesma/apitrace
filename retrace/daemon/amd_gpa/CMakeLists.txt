cmake_minimum_required(VERSION 2.8)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPI-Common
  )

if(EXISTS ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA)
  set (AMD_GPA_SOURCES
    glframe_metrics_amd_gpa.cpp
    glframe_metrics_amd_gpa.hpp
    )
else()
  set (AMD_GPA_SOURCES
    glframe_metrics_amd_gpa_stub.cpp
    )
endif()

add_library(amdgpa
  ${AMD_GPA_SOURCES}
  )

if(EXISTS ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA)
target_link_libraries(amdgpa
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPIGLES/libGPUPerfAPIGLES-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPIGL/libGPUPerfAPIGL-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPICounterGenerator/libGPUPerfAPICounterGenerator-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPI-Common/libGPUPerfAPI-Common-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/DeviceInfo/libDeviceInfo-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPIGLES/libGPUPerfAPIGLES-d.a
  )
endif()

Lint(AMD_GPA_SOURCES)

