cmake_minimum_required(VERSION 3.0)

include(ExternalProject)

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

if(platform_mingw)
  ExternalProject_Add(glm
    URL "https://github.com/g-truc/glm/archive/0.9.9.0.tar.gz"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/glm-install
  )
  include_directories(${PROJECT_BINARY_DIR}/glm-install/include)
endif(platform_mingw)


add_subdirectory(gl_wrapper)
add_subdirectory(src)
add_subdirectory(tools)
if(NOT platform_mingw)
  add_subdirectory(viewer)
  add_subdirectory(testbed)
endif(NOT platform_mingw)
