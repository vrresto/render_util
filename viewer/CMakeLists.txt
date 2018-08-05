set(CXX_SRCS
  viewer_main.cpp
  terrain_viewer.cpp
  heightmap_viewer.cpp
)

add_library(viewer ${CXX_SRCS})

target_link_libraries(viewer render_util glfw)
