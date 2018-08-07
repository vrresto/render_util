set(CXX_SRCS
  terrain.cpp
  terrain_cdlod.cpp
  camera.cpp
  texunits.cpp
  texture_util.cpp
  texture_manager.cpp
  shader.cpp
  shader_util.cpp
  map_textures.cpp
  water.cpp
  image_loader.cpp
  ${PROJECT_SOURCE_DIR}/FastNoise/FastNoise.cpp
)

add_library(render_util ${CXX_SRCS})

target_link_libraries(render_util gl_wrapper)

target_include_directories(render_util PUBLIC
  ${PROJECT_SOURCE_DIR}/FastNoise
  ${PROJECT_SOURCE_DIR}/stb
)