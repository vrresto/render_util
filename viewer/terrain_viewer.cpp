/**
 *    Rendering utilities
 *    Copyright (C) 2018  Jan Lepper
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "viewer_main.h"
#include "scene.h"
#include "camera.h"
#include <render_util/viewer.h>
#include <render_util/render_util.h>
#include <render_util/water.h>
#include <render_util/shader.h>
#include <render_util/shader_util.h>
#include <render_util/texture_manager.h>
#include <render_util/map.h>
#include <render_util/map_textures.h>
#include <render_util/texture_util.h>
#include <render_util/texunits.h>
#include <render_util/image_loader.h>
#include <render_util/image_util.h>
#include <render_util/elevation_map.h>
#include <render_util/camera.h>
#include <render_util/terrain_util.h>
#include <render_util/image_util.h>
#include <gl_wrapper/gl_wrapper.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <memory>
#include <exception>

#include <gl_wrapper/gl_interface.h>
#include <gl_wrapper/gl_functions.h>


using namespace glm;
using namespace std;
using namespace gl_wrapper::gl_functions;
using namespace render_util::viewer;
using namespace render_util;

#include <render_util/skybox.h>


namespace
{


const bool g_terrain_use_lod = true;

const string cache_path = RENDER_UTIL_CACHE_DIR;
const string shader_path = RENDER_UTIL_SHADER_DIR;

const vec4 shore_wave_hz = vec4(0.05, 0.07, 0, 0);

render_util::ShaderProgramPtr createSkyProgram(const render_util::TextureManager &tex_mgr)
{
  return render_util::createSkyProgram(tex_mgr, shader_path);
}


glm::dvec3 hitGround(const Beam &beam_)
{
  auto beam = beam_;

  beam.direction *= -1;

  const glm::vec3 plane_normal = glm::vec3(0,0,1);

  float distance = -dot(plane_normal, beam.origin) / dot(beam.direction, plane_normal);

  if (distance < 0)
    throw std::exception();
  else
    return beam.origin + beam.direction * distance;
}


} // namespace


class TerrainViewerScene : public Scene
{
  vec4 shore_wave_pos = vec4(0);
  vec2 map_size = vec2(0);
  ivec2 mark_pixel_coords = ivec2(0);

  shared_ptr<render_util::MapBase> m_map;

  render_util::TexturePtr curvature_map;
  render_util::TexturePtr atmosphere_map;

  render_util::ShaderProgramPtr sky_program;
//   render_util::ShaderProgramPtr forest_program;

  shared_ptr<render_util::MapLoaderBase> m_map_loader;

  render_util::ImageGreyScale::Ptr m_base_map_land;
  render_util::TexturePtr m_base_map_land_texture;
  ElevationMap::Ptr m_elevation_map_base;

  void updateUniforms(render_util::ShaderProgramPtr program) override;
  void updateBaseWaterMapTexture();
  void buildBaseMap();

public:
  TerrainViewerScene(CreateMapLoaderFunc&);
  ~TerrainViewerScene() override;

  void render(float frame_delta) override;
  void setup() override;
  void mark() override;
  void unmark() override;
  void cursorPos(const glm::dvec2&) override;
  void rebuild() override { buildBaseMap(); }
};


TerrainViewerScene::TerrainViewerScene(CreateMapLoaderFunc &create_map_loader)
{
  cout<<"TerrainViewerScene::TerrainViewerScene"<<endl;

  m_map_loader = create_map_loader(getTextureManager());
  assert(m_map_loader);
}

TerrainViewerScene::~TerrainViewerScene()
{
  auto land_map_flipped = image::flipY(m_base_map_land);
  saveImageToFile(RENDER_UTIL_CACHE_DIR "/base_map_land_editor.tga", land_map_flipped.get());
  std::ofstream s(RENDER_UTIL_CACHE_DIR "/base_map_origin_editor", ios_base::binary);
  s<<"BaseMapOriginX="<<base_map_origin.x<<endl;
  s<<"BaseMapOriginY="<<base_map_origin.y<<endl;
}


void TerrainViewerScene::buildBaseMap()
{
  m_elevation_map_base = m_map_loader->createBaseElevationMap(m_base_map_land);
  base_map_size_m =
    glm::vec2(m_elevation_map_base->getSize() * (int)render_util::HEIGHT_MAP_BASE_METERS_PER_PIXEL);

  updateTerrain(m_elevation_map_base);

  m_map->buildBaseMap(m_elevation_map_base, m_base_map_land);
  m_map->getTextures().bind(getTextureManager());

  updateBaseWaterMapTexture();
}


void TerrainViewerScene::mark()
{
  m_base_map_land->at(mark_pixel_coords) = 255;
  updateBaseWaterMapTexture();
}


void TerrainViewerScene::unmark()
{
  m_base_map_land->at(mark_pixel_coords) = 0;
  updateBaseWaterMapTexture();
}


void TerrainViewerScene::cursorPos(const glm::dvec2 &pos)
{
  auto beam = camera.createBeamThroughViewportCoord(glm::vec2(pos));

  vec3 ground_plane_pos = vec3(0);

  try
  {
    ground_plane_pos = hitGround(beam);
  }
  catch(...)
  {
    return;
  }

  auto base_map_pos_relative = (glm::vec2(ground_plane_pos.x, ground_plane_pos.y) - base_map_origin) / base_map_size_m;
  mark_pixel_coords = base_map_pos_relative * glm::vec2(m_base_map_land->getSize());
  mark_pixel_coords = clamp(mark_pixel_coords, ivec2(0), m_base_map_land->getSize() - ivec2(1));
}


void TerrainViewerScene::updateBaseWaterMapTexture()
{
  setTextureImage(m_base_map_land_texture, m_base_map_land);
}


void TerrainViewerScene::setup()
{
  cout<<"void TerrainViewerScene::setup()"<<endl;

  getTextureManager().setActive(true);

  curvature_map = render_util::createCurvatureTexture(getTextureManager(), cache_path);
  atmosphere_map = render_util::createAmosphereThicknessTexture(getTextureManager(), cache_path);


  sky_program = createSkyProgram(getTextureManager());
//   forest_program = render_util::createShaderProgram("forest", getTextureManager(), shader_path);
//   forest_program = render_util::createShaderProgram("forest_cdlod", getTextureManager(), shader_path);

  base_map_origin = m_map_loader->getBaseMapOrigin();

  m_map = m_map_loader->loadMap();

  m_base_map_land = m_map_loader->createBaseLandMap();

  auto elevation_map = m_map_loader->createElevationMap();
  m_elevation_map_base = m_map_loader->createBaseElevationMap(m_base_map_land);


  assert(elevation_map);
  assert(!m_map->getWaterAnimation().isEmpty());

  createTerrain(elevation_map);

  map_size = glm::vec2(elevation_map->getSize() * m_map->getHeightMapMetersPerPixel());

  assert(map_size != vec2(0));

  cout<<"map size: "<<map_size.x<<","<<map_size.y<<endl;

  CHECK_GL_ERROR();

  if (!m_base_map_land)
    m_base_map_land = image::create<unsigned char>(0, ivec2(128));
  m_base_map_land_texture =
      render_util::createTexture<render_util::ImageGreyScale>(m_base_map_land);
  getTextureManager().bind(TEXUNIT_WATER_MAP_BASE, m_base_map_land_texture);

  buildBaseMap();

  CHECK_GL_ERROR();
  m_map->getTextures().bind(getTextureManager());
  CHECK_GL_ERROR();

  camera.x = map_size.x / 2;
  camera.y = map_size.y / 2;
  camera.z = 10000;
}


void TerrainViewerScene::updateUniforms(render_util::ShaderProgramPtr program)
{
  CHECK_GL_ERROR();

  Scene::updateUniforms(program);

  CHECK_GL_ERROR();

  m_map->getWaterAnimation().updateUniforms(program);

  CHECK_GL_ERROR();

  m_map->getTextures().setUniforms(program);
  program->setUniform("shore_wave_scroll", shore_wave_pos);
  program->setUniform("terrain_height_offset", 0);
  program->setUniform("map_size", map_size);

  auto base_map_size =
    m_elevation_map_base->w() * HEIGHT_MAP_BASE_METERS_PER_PIXEL;

  auto land_map_meters_per_pixel = base_map_size / (float)m_base_map_land->w();

  vec2 mark_coords_m = vec2(mark_pixel_coords) * land_map_meters_per_pixel;
  mark_coords_m += base_map_origin;

  program->setUniform("cursor_pos_ground", mark_coords_m);
  program->setUniform("land_map_meters_per_pixel", land_map_meters_per_pixel);

  CHECK_GL_ERROR();
}


void TerrainViewerScene::render(float frame_delta)
{
  gl::Enable(GL_CULL_FACE);

  if (!pause_animations)
  {
    shore_wave_pos.x = shore_wave_pos.x + (frame_delta * shore_wave_hz.x);
    shore_wave_pos.y = shore_wave_pos.y + (frame_delta * shore_wave_hz.y);
    m_map->getWaterAnimation().update();
  }

  CHECK_GL_ERROR();

  gl::Disable(GL_DEPTH_TEST);

//     gl::DepthMask(GL_FALSE);
  gl::FrontFace(GL_CW);
  gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  getCurrentGLContext()->setCurrentProgram(sky_program);
  updateUniforms(sky_program);

  render_util::drawSkyBox();

  gl::Disable(GL_DEPTH_TEST);
  gl::DepthMask(GL_TRUE);
  gl::FrontFace(GL_CCW);

  gl::Enable(GL_DEPTH_TEST);
  gl::DepthMask(GL_TRUE);

  gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  drawTerrain();
  CHECK_GL_ERROR();


  // forest
#if 0
  const int forest_layers = 5;
  const int forest_layer_repetitions = 1;
  const float forest_layer_height = 2.5;

  terrain_renderer.m_terrain->setDrawDistance(5000.f);
  terrain_renderer.m_terrain->update(camera);

  getCurrentGLContext()->setCurrentProgram(forest_program);
  updateUniforms(forest_program);
  forest_program->setUniformi("forest_layer", 0);
  forest_program->setUniform("terrain_height_offset", 0);
  CHECK_GL_ERROR();

  gl::Enable(GL_BLEND);
  gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl::Disable(GL_CULL_FACE);

//   terrain_renderer.m_terrain->draw(forest_program);
  CHECK_GL_ERROR();

  for (int i = 1; i < forest_layers; i++)
  {
    forest_program->setUniformi("forest_layer", i);

    for (int y = 0; y < forest_layer_repetitions; y++)
    {
      float height = i * forest_layer_height + y * (forest_layer_height / forest_layer_repetitions);
      forest_program->setUniform("terrain_height_offset", height);
      terrain_renderer.m_terrain->draw(forest_program);
      CHECK_GL_ERROR();
    }
  }

  gl::Enable(GL_CULL_FACE);
  gl::Disable(GL_BLEND);
#endif

}

void render_util::viewer::runViewer(CreateMapLoaderFunc &create_map_loader)
{
  auto create_func = [&create_map_loader]
  {
    auto scene = make_shared<TerrainViewerScene>(create_map_loader);
    cout<<"create_func: scene: "<<scene.get()<<endl;
    return scene;
  };

  runApplication(create_func);

  cout<<"exiting..."<<endl;
}
