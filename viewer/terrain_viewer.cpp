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
#include <render_util/aerial_perspective.h>
#include <render_util/viewer.h>
#include <render_util/render_util.h>
#include <render_util/water.h>
#include <render_util/shader.h>
#include <render_util/shader_util.h>
#include <render_util/texture_manager.h>
#include <render_util/map_textures.h>
#include <render_util/texture_util.h>
#include <render_util/texunits.h>
#include <render_util/image_loader.h>
#include <render_util/image_util.h>
#include <render_util/elevation_map.h>
#include <render_util/camera.h>
#include <render_util/terrain_util.h>
#include <render_util/image_util.h>
#include <render_util/cirrus_clouds.h>
#include <render_util/state.h>
#include <render_util/gl_binding/gl_binding.h>
#include <log.h>

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

#include <render_util/gl_binding/gl_interface.h>
#include <render_util/gl_binding/gl_functions.h>


using namespace glm;
using namespace std;
using namespace render_util::gl_binding;
using namespace render_util::viewer;
using namespace render_util;

#include <render_util/skybox.h>


namespace
{


// constexpr auto ATMOSPHERE_TYPE = Atmosphere::DEFAULT;
constexpr auto ATMOSPHERE_TYPE = Atmosphere::PRECOMPUTED;
constexpr auto PRECOMPUTED_LUMINANCE = false;
constexpr auto HAZINESS = 0.0;
constexpr auto SINGLE_MIE_HORIZON_HACK = false;
constexpr auto g_terrain_use_lod = true;
constexpr auto cache_path = RENDER_UTIL_CACHE_DIR;
constexpr auto shader_path = RENDER_UTIL_SHADER_DIR;
constexpr auto ENABLE_AERIAL_PERSPECTIVE = true;

const auto shore_wave_hz = vec4(0.05, 0.07, 0, 0);



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


namespace terrain_viewer
{
  class Map : public MapBase
  {
    TerrainBase::MaterialMap::ConstPtr m_material_map;
    MapTextures m_map_textures;
    WaterAnimation m_water_animation;
    std::shared_ptr<GenericImage> m_cirrus_texture;

  public:
    Map(const TextureManager &texture_manager) : m_map_textures(texture_manager) {}

    MapTextures &getTextures() override { return m_map_textures; }
    WaterAnimation &getWaterAnimation() override { return m_water_animation; }
    void setMaterialMap(TerrainBase::MaterialMap::ConstPtr map) override
    {
      m_material_map = map;
    }

    void setCirrusTexture(std::shared_ptr<GenericImage> texture) override
    {
      m_cirrus_texture = texture;
    }

    std::shared_ptr<GenericImage> getCirrusTexture() { return m_cirrus_texture; }

    TerrainBase::MaterialMap::ConstPtr getMaterialMap() { return m_material_map; }
  };
}


class TerrainViewerScene : public Scene
{
  vec4 shore_wave_pos = vec4(0);
  vec2 map_size = vec2(0);
  ivec2 mark_pixel_coords = ivec2(0);

  unique_ptr<terrain_viewer::Map> m_map;

  render_util::TexturePtr curvature_map;
  render_util::TexturePtr atmosphere_map;

  render_util::ShaderProgramPtr sky_program;
//   render_util::ShaderProgramPtr forest_program;

  shared_ptr<MapLoaderBase> m_map_loader;

  unique_ptr<CirrusClouds> m_cirrus_clouds;

  std::unique_ptr<aerial_perspective::AerialPerspective> m_aerial_perspective;
  bool m_manual_compute_trigger = false;

#if ENABLE_BASE_MAP
  render_util::ImageGreyScale::Ptr m_base_map_land;
  render_util::TexturePtr m_base_map_land_texture;
  ElevationMap::Ptr m_elevation_map_base;
#endif

  void updateUniforms(render_util::ShaderProgramPtr program) override;
  void updateBaseWaterMapTexture();
  void buildBaseMap();
  void computeStep();

public:
  TerrainViewerScene(CreateMapLoaderFunc&);
  ~TerrainViewerScene() override;

  void render(float frame_delta) override;
  void setup() override;
  void mark() override;
  void unmark() override;
  void cursorPos(const glm::dvec2&) override;
  void rebuild() override { buildBaseMap(); }
  void recompute() override;
};


TerrainViewerScene::TerrainViewerScene(CreateMapLoaderFunc &create_map_loader)
{
  LOG_INFO<<"TerrainViewerScene::TerrainViewerScene"<<endl;

  m_map_loader = create_map_loader(getTextureManager());
  assert(m_map_loader);
}

TerrainViewerScene::~TerrainViewerScene()
{
#if ENABLE_BASE_MAP
  auto land_map_flipped = image::flipY(m_base_map_land);
  saveImageToFile(RENDER_UTIL_CACHE_DIR "/base_map_land_editor.tga", land_map_flipped.get());
  std::ofstream s(RENDER_UTIL_CACHE_DIR "/base_map_origin_editor", ios_base::binary);
  s<<"BaseMapOriginX="<<base_map_origin.x<<endl;
  s<<"BaseMapOriginY="<<base_map_origin.y<<endl;
#endif
}


void TerrainViewerScene::recompute()
{
  if (ENABLE_AERIAL_PERSPECTIVE)
    m_aerial_perspective->computeStep(camera, getTextureManager());
}


void TerrainViewerScene::buildBaseMap()
{
#if ENABLE_BASE_MAP
  m_elevation_map_base = m_map_loader->createBaseElevationMap(m_base_map_land);
  base_map_size_m =
    glm::vec2(m_elevation_map_base->getSize() * (int)render_util::HEIGHT_MAP_BASE_METERS_PER_PIXEL);

  updateTerrain(m_elevation_map_base);

  m_map->getTextures().bind(getTextureManager());

  updateBaseWaterMapTexture();
#endif
}


void TerrainViewerScene::mark()
{
#if ENABLE_BASE_MAP
  m_base_map_land->at(mark_pixel_coords) = 255;
  updateBaseWaterMapTexture();
#endif
}


void TerrainViewerScene::unmark()
{
#if ENABLE_BASE_MAP
  m_base_map_land->at(mark_pixel_coords) = 0;
  updateBaseWaterMapTexture();
#endif
}


void TerrainViewerScene::cursorPos(const glm::dvec2 &pos)
{
#if ENABLE_BASE_MAP
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
#endif
}


void TerrainViewerScene::updateBaseWaterMapTexture()
{
#if ENABLE_BASE_MAP
  setTextureImage(m_base_map_land_texture, m_base_map_land);
#endif
}


void TerrainViewerScene::setup()
{
  LOG_INFO<<"void TerrainViewerScene::setup()"<<endl;

  {
    AtmosphereCreationParameters params;
    params.max_cirrus_albedo = 0.4;
    params.precomputed_luminance = PRECOMPUTED_LUMINANCE;
    params.haziness = HAZINESS;
    params.single_mie_horizon_hack = SINGLE_MIE_HORIZON_HACK;

    m_atmosphere = createAtmosphere(ATMOSPHERE_TYPE, getTextureManager(),
                                    RENDER_UTIL_SHADER_DIR,
                                    params);
  }

  getTextureManager().setActive(true);

  curvature_map = render_util::createCurvatureTexture(getTextureManager(), cache_path);
  atmosphere_map = render_util::createAmosphereThicknessTexture(getTextureManager(), cache_path);

  ShaderSearchPath shader_search_path;
  shader_search_path.push_back(string(RENDER_UTIL_SHADER_DIR) + "/" + m_atmosphere->getShaderPath());
  shader_search_path.push_back(RENDER_UTIL_SHADER_DIR);

  auto shader_params = m_atmosphere->getShaderParameters();

  shader_params.set("enable_aerial_perspective", ENABLE_AERIAL_PERSPECTIVE);

  if (ENABLE_AERIAL_PERSPECTIVE)
  {
    auto update_uniforms = [this] (ShaderProgramPtr program)
    {
      updateUniforms(program);
    };
    m_aerial_perspective =
      std::make_unique<aerial_perspective::AerialPerspective>(shader_params,
                                                              update_uniforms,
                                                              shader_search_path,
                                                              getTextureManager());
    shader_params.add(m_aerial_perspective->getShaderParameters());
  }

  sky_program = render_util::createShaderProgram("sky", getTextureManager(),
                                                 shader_search_path, {}, shader_params);
//   forest_program = render_util::createShaderProgram("forest", getTextureManager(), shader_path);
//   forest_program = render_util::createShaderProgram("forest_cdlod", getTextureManager(), shader_path);

  m_map = make_unique<terrain_viewer::Map>(getTextureManager());

  auto elevation_map = m_map_loader->createElevationMap();

  m_map_loader->createMapTextures(m_map.get());

#if ENABLE_BASE_MAP
  base_map_origin = m_map_loader->getBaseMapOrigin();
  m_base_map_land = m_map_loader->createBaseLandMap();
  m_elevation_map_base = m_map_loader->createBaseElevationMap(m_base_map_land);
#endif

  assert(elevation_map);
  assert(!m_map->getWaterAnimation().isEmpty());

  LandTextures land_textures;
  m_map_loader->createLandTextures(land_textures);

  assert(m_map->getMaterialMap());

  m_map->getTextures().setTexture(TEXUNIT_TERRAIN_FAR, land_textures.far_texture);

  createTerrain(elevation_map, m_map->getMaterialMap(), land_textures,
                shader_search_path, shader_params);

  map_size = glm::vec2(elevation_map->getSize() * m_map_loader->getHeightMapMetersPerPixel());

  assert(map_size != vec2(0));

  LOG_INFO<<"map size: "<<map_size.x<<","<<map_size.y<<endl;

  CHECK_GL_ERROR();

#if ENABLE_BASE_MAP
  if (!m_base_map_land)
    m_base_map_land = image::create<unsigned char>(0, ivec2(128));
  m_base_map_land_texture =
      render_util::createTexture<render_util::ImageGreyScale>(m_base_map_land);
  getTextureManager().bind(TEXUNIT_WATER_MAP_BASE, m_base_map_land_texture);

  buildBaseMap();
#endif

  m_cirrus_clouds = make_unique<CirrusClouds>(0.7, getTextureManager(), shader_search_path,
                                              shader_params, 7000, m_map->getCirrusTexture());

  CHECK_GL_ERROR();
  m_map->getTextures().bind(getTextureManager());
  CHECK_GL_ERROR();

  if (ENABLE_AERIAL_PERSPECTIVE)
    m_aerial_perspective->bindTextures(getTextureManager());

  camera.x = map_size.x / 2;
  camera.y = map_size.y / 2;
  camera.z = 10000;

  createControllers();

  camera.setProjection(90, 1.0, 2000.0 * 1000.0);

  {
    std::vector<bool> values = { false, true };
    auto apply = [this] (bool value) { m_manual_compute_trigger = value; };
    m_parameters.addMultipleChoice<bool>("manual_compute_trigger", apply, values);
  }
}


void TerrainViewerScene::updateUniforms(render_util::ShaderProgramPtr program)
{
  CHECK_GL_ERROR();

  Scene::updateUniforms(program);

  CHECK_GL_ERROR();

  m_map->getWaterAnimation().updateUniforms(program);

  CHECK_GL_ERROR();

  if (ENABLE_AERIAL_PERSPECTIVE)
    m_aerial_perspective->updateUniforms(program, getTextureManager());

  m_map->getTextures().setUniforms(program);
  program->setUniform("shore_wave_scroll", shore_wave_pos);
  program->setUniform("terrain_height_offset", 0.f);
  program->setUniform("terrain_base_map_height", 0.f);
  program->setUniform("map_size", map_size);

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

  if (ENABLE_AERIAL_PERSPECTIVE && !m_manual_compute_trigger)
    m_aerial_perspective->computeStep(camera, getTextureManager());

  updateTerrain();

  gl::Disable(GL_DEPTH_TEST);

//     gl::DepthMask(GL_FALSE);
  gl::FrontFace(GL_CW);
  gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  getCurrentGLContext()->setCurrentProgram(sky_program);
  updateUniforms(sky_program);

  gl::MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  render_util::drawSkyBox();

  gl::Disable(GL_DEPTH_TEST);
  gl::DepthMask(GL_TRUE);
  gl::FrontFace(GL_CCW);

  gl::Enable(GL_DEPTH_TEST);
  gl::DepthMask(GL_TRUE);

  gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  drawTerrain();
  CHECK_GL_ERROR();

  {
    const auto original_state = State::fromCurrent();
    StateModifier state(original_state);

    state.setDefaults();
    state.enableBlend(true);

    getCurrentGLContext()->setCurrentProgram(m_cirrus_clouds->getProgram());
    updateUniforms(m_cirrus_clouds->getProgram());
    m_cirrus_clouds->getProgram()->setUniform("is_far_camera", true);
    m_cirrus_clouds->draw(state, camera);
    m_cirrus_clouds->getProgram()->setUniform("is_far_camera", false);
    m_cirrus_clouds->draw(state, camera);
  }

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

void render_util::viewer::runViewer(CreateMapLoaderFunc &create_map_loader, string app_name)
{
  auto create_func = [&create_map_loader]
  {
    auto scene = make_shared<TerrainViewerScene>(create_map_loader);
    LOG_INFO<<"create_func: scene: "<<scene.get()<<endl;
    return scene;
  };

  runApplication(create_func, app_name);

  LOG_INFO<<"exiting..."<<endl;
}
