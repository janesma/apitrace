cmake_minimum_required(VERSION 2.8)

include_directories (
  ${CMAKE_SOURCE_DIR}/retrace/daemon
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPI-Common
  )

set (AMD_GPA_SOURCES
  glframe_metrics_amd_gpa.cpp
  glframe_metrics_amd_gpa.hpp
  )

add_library(amdgpa
  ${AMD_GPA_SOURCES}
  )

target_link_libraries(amdgpa
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPIGLES/libGPUPerfAPIGLES-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPIGL/libGPUPerfAPIGL-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPICounterGenerator/libGPUPerfAPICounterGenerator-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPI-Common/libGPUPerfAPI-Common-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/DeviceInfo/libDeviceInfo-d.a
  ${CMAKE_SOURCE_DIR}/retrace/daemon/amd_gpa/GPA/Src/GPUPerfAPIGLES/libGPUPerfAPIGLES-d.a
  )
  
Lint(AMD_GPA_SOURCES)

