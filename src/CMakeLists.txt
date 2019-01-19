set(CXX_SRCS
#   terrain.cpp
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
  image_writer.cpp
  util.cpp
  terrain_util.cpp
  gl_context.cpp
  globals.cpp
  ${PROJECT_SOURCE_DIR}/_modules/FastNoise/FastNoise.cpp
)

set_source_files_properties(
  image_writer.cpp
  PROPERTIES
    COMPILE_FLAGS "-Wno-missing-declarations"
)

add_library(render_util ${CXX_SRCS})

target_link_libraries(render_util render_util_tools gl_binding)

target_include_directories(render_util PUBLIC
  ${PROJECT_SOURCE_DIR}/_modules/FastNoise
  ${PROJECT_SOURCE_DIR}/_modules/stb
)
