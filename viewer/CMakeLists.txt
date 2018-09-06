if(platform_mingw)
  ExternalProject_Get_Property(glfw_source SOURCE_DIR)
  include_directories(${SOURCE_DIR}/include)
  ExternalProject_Get_Property(glfw_source BINARY_DIR)
  set(glfw_lib_dir ${BINARY_DIR}/src)
endif()

# add_definitions(
#   -DRENDER_UTIL_SHADER_DIR="${PROJECT_SOURCE_DIR}/shaders"
#   -DRENDER_UTIL_RESOURCE_DIR="${PROJECT_BINARY_DIR}/tools"
# )

set(CXX_SRCS
  viewer_main.cpp
  terrain_viewer.cpp
  heightmap_viewer.cpp
)

add_library(viewer ${CXX_SRCS})

if(platform_mingw)
  target_link_libraries(viewer -L${glfw_lib_dir})
endif()

target_link_libraries(viewer
  render_util
  glfw3
)

if(platform_mingw)
  target_link_libraries(viewer gdi32)
endif()
