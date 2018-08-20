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
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/glm-install
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_CXX_COMPILER_WORKS=1
  )
  include_directories(${PROJECT_BINARY_DIR}/glm-install/include)
endif(platform_mingw)

if(no_std_thread)
  add_definitions(-DNO_STD_THREAD)
else(no_std_thread)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  link_libraries(pthread)
endif(no_std_thread)

add_subdirectory(gl_wrapper)
add_subdirectory(src)
if(NOT no_tools)
  add_subdirectory(tools)
endif(NOT no_tools)
if(NOT platform_mingw)
  add_subdirectory(viewer)
  add_subdirectory(testbed)
endif(NOT platform_mingw)
