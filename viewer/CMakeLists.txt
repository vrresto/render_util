if(platform_mingw)
  ExternalProject_Get_Property(glfw_source SOURCE_DIR)
  include_directories(${SOURCE_DIR}/include)
  ExternalProject_Get_Property(glfw_source BINARY_DIR)
  set(glfw_lib_dir ${BINARY_DIR}/src)
endif()

set(CXX_SRCS
  viewer_main.cpp
  terrain_viewer.cpp
  heightmap_viewer.cpp
)

add_library(viewer ${CXX_SRCS})

target_link_libraries(viewer render_util)

if(platform_mingw)
  target_link_libraries(viewer -L${glfw_lib_dir})
  target_link_libraries(viewer glfw3)
  target_link_libraries(viewer gdi32)
else()
  target_link_libraries(viewer glfw)
endif()
