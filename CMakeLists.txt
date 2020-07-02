cmake_minimum_required(VERSION 3.0)

include(ExternalProject)

project(render_util)

if(NOT renderutil_shader_dir)
  set(renderutil_shader_dir "shaders")
endif()

if(NOT renderutil_cache_dir)
  set(renderutil_cache_dir "cache")
endif()

if(NOT DEFINED use_plog)
  set(use_plog 1)
endif()

if(NOT plog_dir)
  set(plog_dir ${PROJECT_SOURCE_DIR}/_modules/plog)
endif()

if(NOT DEFINED use_unix_console)
  set(use_unix_console 0)
endif()

if (NOT enable_gl_debug_callback)
  set(enable_gl_debug_callback 0)
endif()

if (NOT DEFINED enable_atmosphere_precomputed_plot_parameterisation)
  set(enable_atmosphere_precomputed_plot_parameterisation 0)
endif()


configure_file(include/render_util/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/render_util/config.h
)

if(mesa_path)
  set(MESA_PATH ${mesa_path})
else()
  ExternalProject_Add(mesa
    URL "https://mesa.freedesktop.org/archive/mesa-18.1.6.tar.xz"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
  )
  ExternalProject_Get_Property(mesa SOURCE_DIR)
  set(MESA_PATH ${SOURCE_DIR})
endif()

include(cflags.cmake)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_BINARY_DIR}
)

ExternalProject_Add(glm
  URL "https://github.com/g-truc/glm/archive/0.9.9.0.tar.gz"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(glm SOURCE_DIR)
include_directories(${SOURCE_DIR})

ExternalProject_Add(half
  URL "https://downloads.sourceforge.net/project/half/half/2.1.0/half-2.1.0.zip"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
ExternalProject_Get_Property(half SOURCE_DIR)
include_directories(${SOURCE_DIR}/include)


if(platform_mingw)
  if (render_util_build_viewer)
    ExternalProject_Add(glfw_source
      URL "https://github.com/glfw/glfw/releases/download/3.2.1/glfw-3.2.1.zip"
      CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=CMake/i686-w64-mingw32.cmake -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
      INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(glfw_source SOURCE_DIR)
    set(glfw_source_dir ${SOURCE_DIR})
  endif()
endif()

if(no_std_thread)
  add_definitions(-DNO_STD_THREAD)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
  link_libraries(pthread)
endif()

add_definitions(
  -DRENDER_UTIL_SHADER_DIR="${renderutil_shader_dir}"
  -DRENDER_UTIL_CACHE_DIR="${renderutil_cache_dir}"
)

add_subdirectory(gl_binding)
add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(util)

if(render_util_build_viewer)
  add_subdirectory(viewer)

#   if(NOT platform_mingw)
#     add_subdirectory(testbed)
#   endif()
endif()

if(platform_mingw)
  add_dependencies(render_util_tools glm)
endif()


if(NOT renderutil_base_dir)
  set(renderutil_base_dir "render_util")
endif()

set(documentation_base_dir "${renderutil_base_dir}/documentation")
set(thirdparty_base_dir "${renderutil_base_dir}/documentation/thirdparty")


install(DIRECTORY shaders
  DESTINATION ${renderutil_base_dir}
)

install(FILES
  precomputed_atmospheric_scattering/atmosphere/definitions.glsl
  precomputed_atmospheric_scattering/atmosphere/constants.glsl
  precomputed_atmospheric_scattering/atmosphere/functions.glsl
  DESTINATION ${renderutil_base_dir}/shaders/atmosphere_precomputed
)

install(FILES LICENSE
  DESTINATION ${documentation_base_dir}
  RENAME LICENSE.txt
)

install(FILES precomputed_atmospheric_scattering/LICENSE
  DESTINATION ${thirdparty_base_dir}/precomputed_atmospheric_scattering
#   RENAME precomputed_atmospheric_scattering_LICENSE.txt
)
install(FILES precomputed_atmospheric_scattering/README.md
  DESTINATION ${thirdparty_base_dir}/precomputed_atmospheric_scattering
#   RENAME precomputed_atmospheric_scattering_README.md
)

install(FILES text_renderer/LICENSE
  DESTINATION ${thirdparty_base_dir}/text_renderer
#   RENAME text_renderer_LICENSE.txt
)

install(FILES _modules/FastNoise/LICENSE
  DESTINATION ${thirdparty_base_dir}/FastNoise
#   RENAME FastNoise_LICENSE.txt
)
install(FILES _modules/FastNoise/README.md
  DESTINATION ${thirdparty_base_dir}/FastNoise
#   RENAME FastNoise_README.md
)

install(FILES _modules/stb/README.md
  DESTINATION ${thirdparty_base_dir}/stb
#   RENAME stb_README.md
)

install(FILES ${plog_dir}/LICENSE
  DESTINATION ${thirdparty_base_dir}/plog
#   RENAME plog_LICENSE.txt
)
install(FILES ${plog_dir}/README.md
  DESTINATION ${thirdparty_base_dir}/plog
#   RENAME plog_README.md
)

if(glfw_source_dir)
  install(FILES ${glfw_source_dir}/COPYING.txt
    DESTINATION ${thirdparty_base_dir}/GLFW
#     RENAME GLFW_COPYING.txt
  )
  install(FILES ${glfw_source_dir}/README.md
    DESTINATION ${thirdparty_base_dir}/GLFW
#     RENAME GLFW_README.md
  )
endif()
