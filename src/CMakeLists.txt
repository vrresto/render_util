set(CXX_SRCS
  terrain/terrain_cdlod_base.cpp
  terrain/terrain_cdlod.cpp
  terrain/terrain_util.cpp
  terrain/land.cpp
  terrain/forest.cpp
  terrain/water.cpp
  terrain/noise.cpp
  atmosphere.cpp
  camera.cpp
  texunits.cpp
  texture_util.cpp
  texture_manager.cpp
  shader.cpp
  shader_util.cpp
  image_loader.cpp
  image_writer.cpp
  util.cpp
  gl_context.cpp
  globals.cpp
  atmosphere_precomputed.cpp
  quad_2d.cpp
  text_display.cpp
  cirrus.cpp
  uv_dome.cpp
  grid_mesh.cpp
  indexed_mesh.cpp
  vao.cpp
  state.cpp
  ${PROJECT_SOURCE_DIR}/_modules/FastNoise/FastNoise.cpp
  ${PROJECT_SOURCE_DIR}/precomputed_precomputed_atmospheric_scattering/atmosphere/model.cc
  ${PROJECT_SOURCE_DIR}/text_renderer/text_renderer.cc
)

set_source_files_properties(
  image_writer.cpp
  PROPERTIES
    COMPILE_FLAGS "-Wno-missing-declarations"
)

add_library(render_util ${CXX_SRCS})

target_link_libraries(render_util
  render_util_tools
  render_util_util
  gl_binding
)

target_include_directories(render_util PUBLIC
  ${PROJECT_SOURCE_DIR}/_modules/FastNoise
  ${PROJECT_SOURCE_DIR}/_modules/stb
  ${CMAKE_CURRENT_SOURCE_DIR}
)
