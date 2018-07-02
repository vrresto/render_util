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
#include <viewer.h>
#include <engine.h>
#include <engine/water.h>
#include <engine/shader.h>
#include <engine/texture_manager.h>
#include <engine/map.h>
#include <engine/map_textures.h>
#include <engine/terrain.h>
#include <engine/terrain_cdlod.h>
#include <engine/texture_util.h>
#include <engine/texunits.h>
#include <engine/image_loader.h>
#include <engine/elevation_map.h>
#include <engine/camera.h>
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
#include <engine/shader.h>
#include <engine/shader_util.h>

using namespace glm;
using namespace std;
using namespace gl_wrapper::gl_functions;
using engine::ShaderProgram;

#include <engine/skybox.h>

namespace
{


// typedef engine::Terrain Terrain;
typedef engine::TerrainCDLOD Terrain;

const string resource_path = engine::getResourcePath() + "/build/native/";
const string shader_path = engine::getResourcePath() + "/shaders";

const vec4 shore_wave_hz = vec4(0.05, 0.07, 0, 0);

engine::ShaderProgramPtr createSkyProgram(const engine::TextureManager &tex_mgr)
{
  return engine::createSkyProgram(tex_mgr, shader_path);
}

engine::ShaderProgramPtr createTerrainProgram(const string &name, const engine::TextureManager &tex_mgr)
{
  engine::ShaderProgramPtr terrain_program;

  CHECK_GL_ERROR();

  map<unsigned int, string> attribute_locations = { { 2, "attrib_pos" } };

  terrain_program = engine::createShaderProgram(name, tex_mgr, shader_path, attribute_locations);

  CHECK_GL_ERROR();

  return terrain_program;
}


} // namespace


class TerrainViewerScene : public Scene
{
  vec4 shore_wave_pos = vec4(0);
  vec2 map_size = vec2(0);

  string map_path;
  shared_ptr<engine::Map> map;
  engine::WaterAnimation water_animation;

  engine::TexturePtr curvature_map;
  engine::TexturePtr atmosphere_map;

  engine::ShaderProgramPtr sky_program;
  engine::ShaderProgramPtr terrain_program;
  engine::ShaderProgramPtr forest_program;

  shared_ptr<MapLoaderBase> map_loader;
  engine::TextureManager texture_manager = engine::TextureManager(0);

  void updateUniforms(engine::ShaderProgramPtr program) override;

  engine::TextureManager &getTextureManager() { return texture_manager; }


public:
  TerrainViewerScene(shared_ptr<MapLoaderBase> map_loader, const string &map_path) :
    map_loader(map_loader), map_path(map_path) {}
  void render(float frame_delta) override;
  void setup() override;
};


void TerrainViewerScene::setup()
{
  getTextureManager().setActive(true);

  sky_program = createSkyProgram(getTextureManager());
//   terrain_program = createTerrainProgram("terrain_simple", getTextureManager());
  terrain_program = createTerrainProgram("terrain_cdlod", getTextureManager());
//   forest_program = engine::createShaderProgram("forest", getTextureManager(), shader_path);
  forest_program = engine::createShaderProgram("forest_cdlod", getTextureManager(), shader_path);

  map.reset(new engine::Map);
  map->terrain.reset(new Terrain);
  map->terrain->setTextureManager(&getTextureManager());
  map->textures.reset(new engine::MapTextures(getTextureManager()));

  water_animation.createTextures(map->textures.get());

  map_loader->loadMap(map_path, *map);
#if 0
  {
    int elevation_map_width = 5000;
    int elevation_map_height = 5000;
    std::vector<float> data(elevation_map_width * elevation_map_height);
    engine::ElevationMap elevation_map(elevation_map_width, elevation_map_height, data);
    map->terrain->build(&elevation_map);
  }
#endif

  map_size = map->size;

  cout<<"map size: "<<map_size.x<<","<<map_size.y<<endl;

  curvature_map = engine::createCurvatureTexture(getTextureManager(), resource_path);
  atmosphere_map = engine::createAmosphereThicknessTexture(getTextureManager(), resource_path);

  CHECK_GL_ERROR();

  map->textures->bind();
  CHECK_GL_ERROR();

  CHECK_GL_ERROR();
  
  camera.x = map_size.x / 2;
  camera.y = map_size.y / 2;
  camera.z = 10000;
}

void TerrainViewerScene::updateUniforms(engine::ShaderProgramPtr program)
{
  CHECK_GL_ERROR();

  Scene::updateUniforms(program);

  CHECK_GL_ERROR();

  water_animation.updateUniforms(program);

  CHECK_GL_ERROR();
  
  map->textures->setUniforms(program);
  program->setUniform("shore_wave_scroll", shore_wave_pos);
  program->setUniform("terrain_height_offset", 0);
  program->setUniform("map_size", map_size);

  CHECK_GL_ERROR();
}

void TerrainViewerScene::render(float frame_delta)
{
  glEnable(GL_CULL_FACE);

  if (!pause_animations)
  {
    shore_wave_pos.x = shore_wave_pos.x + (frame_delta * shore_wave_hz.x);
    shore_wave_pos.y = shore_wave_pos.y + (frame_delta * shore_wave_hz.y);
    water_animation.update();
  }
  
//   camera.setZFar(2300000.0);
//   camera.applyFov();

  CHECK_GL_ERROR();

  glDisable(GL_DEPTH_TEST);

//     glDepthMask(GL_FALSE);
  glFrontFace(GL_CW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glUseProgram(sky_program->getId());
  updateUniforms(sky_program);

  engine::drawSkyBox();

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glFrontFace(GL_CCW);
//     glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

//     glUseProgram(getTerrainNoMapProgram()->getId());
//     updateUniforms(getTerrainNoMapProgram());

//     terrain_far->draw();

//     glEnable(GL_DEPTH_TEST);
//     glDepthMask(GL_TRUE);
//   terrain_color = mix(vec4(0.0, 0.6, 0.0, 1.0), vec4(vec3(0.8), 1.0), 0.5);
//     terrain_color = vec4(0,0,0,0);
//     terrain_color = vec4(1,1,1,1);


  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

//     terrain_color = vec4(0,0,0,0);
//     terrain_color = vec4(1,1,1,1);

  map->terrain->setDrawDistance(0);
  map->terrain->update(camera.getPos());
  
  glUseProgram(terrain_program->getId());
  updateUniforms(terrain_program);
  CHECK_GL_ERROR();
  
  map->terrain->draw(terrain_program);
  CHECK_GL_ERROR();


  // forest
#if 0
  const int forest_layers = 5;
  const int forest_layer_repetitions = 1;
  const float forest_layer_height = 2.5;

  map->terrain->setDrawDistance(5000.f);
  map->terrain->update(camera.getPos());
  
  glUseProgram(forest_program->getId());
  updateUniforms(forest_program);
  forest_program->setUniformi("forest_layer", 0);
  forest_program->setUniform("terrain_height_offset", 0);
  CHECK_GL_ERROR();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);

//   map->terrain->draw(forest_program);
  CHECK_GL_ERROR();
  
  for (int i = 1; i < forest_layers; i++)
  {
    forest_program->setUniformi("forest_layer", i);

    for (int y = 0; y < forest_layer_repetitions; y++)
    {
      float height = i * forest_layer_height + y * (forest_layer_height / forest_layer_repetitions);
      forest_program->setUniform("terrain_height_offset", height);
      map->terrain->draw(forest_program);
      CHECK_GL_ERROR();
    }
  }
  
  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);
#endif

}

void runViewer(shared_ptr<MapLoaderBase> map_loader, const std::string &map_path)
{
  auto create_func = [map_loader, map_path]
  {
    return make_shared<TerrainViewerScene>(map_loader, map_path);
  };

  runApplication(create_func);

  cout<<"exiting..."<<endl;
}
