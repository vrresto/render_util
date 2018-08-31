add_definitions(
  -DRENDER_UTIL_SHADER_DIR="${PROJECT_SOURCE_DIR}/shaders"
  -DRENDER_UTIL_RESOURCE_DIR="${PROJECT_BINARY_DIR}/tools"
)

set(CXX_SRCS
  viewer_main.cpp
  terrain_viewer.cpp
  heightmap_viewer.cpp
)

add_library(viewer ${CXX_SRCS})

target_link_libraries(viewer render_util glfw)
