cmake_minimum_required(VERSION 3.0)

include(ExternalProject)

project(render_util)

if(NOT renderutil_shader_dir)
  set(renderutil_shader_dir "shaders")
endif()

if(NOT renderutil_cache_dir)
  set(renderutil_cache_dir "cache")
endif()


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

  if (render_util_build_viewer)
    ExternalProject_Add(glfw_source
      URL "https://github.com/glfw/glfw/releases/download/3.2.1/glfw-3.2.1.zip"
      CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=CMake/i686-w64-mingw32.cmake -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
      INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(glfw_source SOURCE_DIR)
    set(glfw_source_dir ${SOURCE_DIR})
  endif()

endif(platform_mingw)

if(no_std_thread)
  add_definitions(-DNO_STD_THREAD)
else(no_std_thread)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  link_libraries(pthread)
endif(no_std_thread)

add_definitions(
  -DRENDER_UTIL_SHADER_DIR="${renderutil_shader_dir}"
  -DRENDER_UTIL_CACHE_DIR="${renderutil_cache_dir}"
)

add_subdirectory(gl_binding)
add_subdirectory(src)
add_subdirectory(tools)

if(render_util_build_viewer)
  add_subdirectory(viewer)

  if(NOT platform_mingw)
    add_subdirectory(testbed)
  endif(NOT platform_mingw)
endif()

if(platform_mingw)
  add_dependencies(render_util_tools glm)
endif(platform_mingw)


if(NOT renderutil_base_dir)
  set(renderutil_base_dir "render_util")
endif()

install(FILES LICENSE
  DESTINATION ${renderutil_base_dir}
  RENAME LICENSE.txt
)

install(FILES _modules/FastNoise/LICENSE
  DESTINATION ${renderutil_base_dir}
  RENAME FastNoise_LICENSE.txt
)

install(FILES _modules/FastNoise/README.md
  DESTINATION ${renderutil_base_dir}
  RENAME FastNoise_README.md
)

install(FILES _modules/stb/README.md
  DESTINATION ${renderutil_base_dir}
  RENAME stb_README.md
)

if(glfw_source_dir)
  install(FILES ${glfw_source_dir}/COPYING.txt
    DESTINATION ${renderutil_base_dir}
    RENAME GLFW_COPYING.txt
  )
  install(FILES ${glfw_source_dir}/README.md
    DESTINATION ${renderutil_base_dir}
    RENAME GLFW_README.md
  )
endif()
