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

#include "scene.h"
#include "camera.h"
#include "viewer_main.h"
#include <viewer.h>
#include <render_util.h>
#include <render_util/shader.h>
#include <render_util/shader_util.h>
#include <render_util/elevation_map.h>
#include <render_util/map.h>
#include <render_util/map_textures.h>
#include <render_util/terrain.h>
#include <render_util/terrain_cdlod.h>
#include <render_util/texture_util.h>
#include <render_util/texunits.h>
#include <render_util/image_loader.h>
#include <render_util/camera.h>
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
using render_util::ShaderProgram;

#include <render_util/skybox.h>


namespace
{

const string resource_path = render_util::getResourcePath() + "/build/native/";
const string shader_path = render_util::getResourcePath() + "/shaders";

render_util::ShaderProgramPtr createSkyProgram(const render_util::TextureManager &tex_mgr)
{
  return render_util::createSkyProgram(tex_mgr, shader_path);
}

render_util::ShaderProgramPtr createTerrainProgram(const string &name, const render_util::TextureManager &tex_mgr)
{
  render_util::ShaderProgramPtr program;

  CHECK_GL_ERROR();

  map<unsigned int, string> attribute_locations = { { 2, "attrib_pos" } };

  program = render_util::createShaderProgram(name, tex_mgr, shader_path, attribute_locations);

  CHECK_GL_ERROR();

  return program;
}


} // namespace


class HeightMapViewerScene : public Scene
{
  shared_ptr<render_util::TerrainBase> m_terrain;
	render_util::Image<float>::ConstPtr m_height_map;
  render_util::TextureManager texture_manager = render_util::TextureManager(0);
  render_util::ShaderProgramPtr m_sky_program;
  render_util::ShaderProgramPtr m_terrain_program;
  render_util::TexturePtr curvature_map;
  render_util::TexturePtr atmosphere_map;

public:
	HeightMapViewerScene(render_util::Image<float>::ConstPtr height_map) : m_height_map(height_map) {}

  void setup() override;
  void render(float frame_delta) override;
  void updateUniforms(render_util::ShaderProgramPtr program) override;

  render_util::TextureManager &getTextureManager() { return texture_manager; }
};

void HeightMapViewerScene::setup()
{
  getTextureManager().setActive(true);

  m_sky_program = createSkyProgram(getTextureManager());
  m_terrain_program = createTerrainProgram("terrain_cdlod_simple", getTextureManager());
  
  curvature_map = render_util::createCurvatureTexture(getTextureManager(), resource_path);
  atmosphere_map = render_util::createAmosphereThicknessTexture(getTextureManager(), resource_path);

  render_util::ElevationMap elevation_map(m_height_map);

  m_terrain = make_shared<render_util::TerrainCDLOD>();
  m_terrain->setTextureManager(&getTextureManager());
  m_terrain->build(&elevation_map);

  camera.z = 10000;
  sun_azimuth = 20;

  CHECK_GL_ERROR();
}

void HeightMapViewerScene::updateUniforms(render_util::ShaderProgramPtr program)
{
  CHECK_GL_ERROR();
  Scene::updateUniforms(program);
  CHECK_GL_ERROR();
  program->setUniform("terrain_height_offset", 0);
  CHECK_GL_ERROR();
  vec2 map_size = m_height_map->size() * 200;
  program->setUniform("map_size", map_size);
  CHECK_GL_ERROR();
}

void HeightMapViewerScene::render(float frame_delta)
{
  CHECK_GL_ERROR();

  glEnable(GL_CULL_FACE);

  glDisable(GL_DEPTH_TEST);
  glFrontFace(GL_CW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glUseProgram(m_sky_program->getId());
  updateUniforms(m_sky_program);
  render_util::drawSkyBox();


  glFrontFace(GL_CCW);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  m_terrain->update(camera.getPos());

  updateUniforms(m_terrain_program);
  glUseProgram(m_terrain_program->getId());

  m_terrain->draw(m_terrain_program);

  CHECK_GL_ERROR();
}

void runHeightMapViewer(render_util::Image<float>::ConstPtr height_map)
{
  auto create_func = [height_map]
  {
    return make_shared<HeightMapViewerScene>(height_map);
  };

  runApplication(create_func);

  cout<<"exiting..."<<endl;
}
