cmake_minimum_required(VERSION 3.0)

include(ExternalProject)

project(render_util)

if(NOT renderutil_data_dir)
  set(renderutil_data_dir "data")
endif(NOT renderutil_data_dir)


if(mesa_path)
  set(MESA_PATH ${mesa_path})
else(mesa_path)
  ExternalProject_Add(mesa
    URL "https://mesa.freedesktop.org/archive/mesa-18.1.6.tar.xz"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
  ExternalProject_Get_Property(mesa SOURCE_DIR)
  set(MESA_PATH ${SOURCE_DIR})
endif(mesa_path)

include(cflags.cmake)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}
)

if(platform_mingw)
  ExternalProject_Add(glm
    URL "https://github.com/g-truc/glm/archive/0.9.9.0.tar.gz"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
  ExternalProject_Get_Property(glm SOURCE_DIR)
  include_directories(${SOURCE_DIR})
endif(platform_mingw)

if(no_std_thread)
  add_definitions(-DNO_STD_THREAD)
else(no_std_thread)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  link_libraries(pthread)
endif(no_std_thread)

add_subdirectory(gl_wrapper)
add_subdirectory(src)
add_subdirectory(tools)
if(NOT platform_mingw)
  add_subdirectory(viewer)
  add_subdirectory(testbed)
endif(NOT platform_mingw)

if(platform_mingw)
  add_dependencies(render_util_tools glm)
endif(platform_mingw)


install(FILES LICENSE
  DESTINATION ${renderutil_data_dir}
  RENAME LICENSE.txt
)

install(FILES _modules/FastNoise/LICENSE
  DESTINATION ${renderutil_data_dir}
  RENAME LICENSE_FastNoise.txt
)

install(FILES _modules/FastNoise/README.md
  DESTINATION ${renderutil_data_dir}
  RENAME README_FastNoise.md
)

install(FILES _modules/stb/README.md
  DESTINATION ${renderutil_data_dir}
  RENAME README_stb.md
)
