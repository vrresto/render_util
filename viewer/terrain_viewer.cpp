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
#include <render_util/elevation_map.h>
#include <render_util/camera.h>
#include <render_util/terrain_util.h>
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


} // namespace



class TerrainViewerScene : public Scene
{
  vec4 shore_wave_pos = vec4(0);
  vec2 map_size = vec2(0);

  shared_ptr<render_util::Map> map;

  render_util::TexturePtr curvature_map;
  render_util::TexturePtr atmosphere_map;

  render_util::ShaderProgramPtr sky_program;
//   render_util::ShaderProgramPtr forest_program;

  shared_ptr<render_util::MapLoaderBase> map_loader;

  void updateUniforms(render_util::ShaderProgramPtr program) override;

public:
  TerrainViewerScene(shared_ptr<render_util::MapLoaderBase> map_loader) :
    map_loader(map_loader)
  {
    cout<<"TerrainViewerScene()"<<endl;
  }
  void render(float frame_delta) override;
  void setup() override;
};


void TerrainViewerScene::setup()
{
  cout<<"void TerrainViewerScene::setup()"<<endl;

  getTextureManager().setActive(true);

  sky_program = createSkyProgram(getTextureManager());
//   forest_program = render_util::createShaderProgram("forest", getTextureManager(), shader_path);
//   forest_program = render_util::createShaderProgram("forest_cdlod", getTextureManager(), shader_path);

  map = make_shared<Map>();
  map->textures = make_shared<MapTextures>(getTextureManager());
  map->water_animation = make_shared<WaterAnimation>();

  ElevationMap::Ptr elevation_map;
  ElevationMap::Ptr elevation_map_base;

  if (m_use_base_map)
    map_loader->loadMap(*map, m_use_base_water_map, elevation_map, &elevation_map_base);
  else
    map_loader->loadMap(*map, m_use_base_water_map, elevation_map);

  assert(elevation_map);
  if(m_use_base_map)
  {
    assert(elevation_map_base);
  }

  assert(!map->water_animation->isEmpty());

  createTerrain(elevation_map, elevation_map_base);
#if 0
  {
    int elevation_map_width = 5000;
    int elevation_map_height = 5000;
    std::vector<float> data(elevation_map_width * elevation_map_height);
    render_util::ElevationMap elevation_map(elevation_map_width, elevation_map_height, data);
    terrain_renderer.m_terrain->build(&elevation_map);
  }
#endif

  map_size = map->size;
  base_map_origin = map->base_map_origin;

  cout<<"map size: "<<map_size.x<<","<<map_size.y<<endl;

  curvature_map = render_util::createCurvatureTexture(getTextureManager(), cache_path);
  atmosphere_map = render_util::createAmosphereThicknessTexture(getTextureManager(), cache_path);

  CHECK_GL_ERROR();

  map->textures->bind(getTextureManager());
  CHECK_GL_ERROR();

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

  map->water_animation->updateUniforms(program);

  CHECK_GL_ERROR();

  map->textures->setUniforms(program);
  program->setUniform("shore_wave_scroll", shore_wave_pos);
  program->setUniform("terrain_height_offset", 0);
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
    map->water_animation->update();
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

void render_util::viewer::runViewer(shared_ptr<MapLoaderBase> map_loader)
{
  cout<<"render_util::viewer::runViewer: map loader: "<<map_loader.get()<<endl;

  auto create_func = [map_loader]
  {
    cout<<"create_func: map loader: "<<map_loader.get()<<endl;
    auto scene = make_shared<TerrainViewerScene>(map_loader);
    cout<<"create_func: scene: "<<scene.get()<<endl;
    return scene;
  };

  runApplication(create_func);

  cout<<"exiting..."<<endl;
}
