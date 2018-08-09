cmake_minimum_required(VERSION 3.0)
project(render_util)

if(NOT renderutil_data_dir)
  set(renderutil_data_dir "data")
endif(NOT renderutil_data_dir)

set(MESA_PATH ${PROJECT_SOURCE_DIR}/mesa)

include(cflags.cmake)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}
)

add_subdirectory(gl_wrapper)
add_subdirectory(src)
add_subdirectory(viewer)
add_subdirectory(testbed)
add_subdirectory(tools)
