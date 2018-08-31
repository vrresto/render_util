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
#include <render_util/viewer.h>
#include <render_util/render_util.h>
#include <render_util/shader.h>
#include <render_util/shader_util.h>
#include <render_util/elevation_map.h>
#include <render_util/map.h>
#include <render_util/map_textures.h>
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
using namespace render_util::viewer;
using render_util::ShaderProgram;
using render_util::TerrainBase;
using render_util::viewer::ShaderProgramFactory;;


#include <render_util/skybox.h>


namespace
{

const string resource_path = render_util::getResourcePath() + "/build/native/";
const string shader_path = render_util::getResourcePath() + "/shaders";

render_util::ShaderProgramPtr createSkyProgram(const render_util::TextureManager &tex_mgr)
{
  return render_util::createSkyProgram(tex_mgr, shader_path);
}


} // namespace


class HeightMapViewerScene : public Scene
{
  shared_ptr<render_util::TerrainBase> m_terrain;
	render_util::Image<float>::ConstPtr m_height_map;
  render_util::TextureManager texture_manager = render_util::TextureManager(0);
  render_util::ShaderProgramPtr m_sky_program;
  render_util::ShaderProgramPtr m_terrain_program;
  render_util::TexturePtr m_curvature_map;
  render_util::TexturePtr m_atmosphere_map;
  util::Factory<TerrainBase> m_terrain_factory;
  ShaderProgramFactory m_terrain_program_factory;

public:
	HeightMapViewerScene(render_util::Image<float>::ConstPtr height_map,
                       util::Factory<TerrainBase> terrain_factory,
                       ShaderProgramFactory terrain_program_factory) :
    m_height_map(height_map),
    m_terrain_factory(terrain_factory),
    m_terrain_program_factory(terrain_program_factory)
  {
  }

  void setup() override;
  void render(float frame_delta) override;
  void updateUniforms(render_util::ShaderProgramPtr program) override;

  render_util::TextureManager &getTextureManager() { return texture_manager; }
};

void HeightMapViewerScene::setup()
{
  assert(m_height_map);

  getTextureManager().setActive(true);

  m_sky_program = createSkyProgram(getTextureManager());
  m_terrain_program = m_terrain_program_factory(getTextureManager());

  assert(m_sky_program);
  assert(m_terrain_program);

  m_curvature_map = render_util::createCurvatureTexture(getTextureManager(), resource_path);
  m_atmosphere_map = render_util::createAmosphereThicknessTexture(getTextureManager(), resource_path);

  assert(m_curvature_map);
  assert(m_atmosphere_map);

  render_util::ElevationMap elevation_map(m_height_map);

  m_terrain = m_terrain_factory();
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

  gl::Enable(GL_CULL_FACE);

  gl::Disable(GL_DEPTH_TEST);
  gl::FrontFace(GL_CW);
  gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  gl::UseProgram(m_sky_program->getId());
  updateUniforms(m_sky_program);
  render_util::drawSkyBox();


  gl::FrontFace(GL_CCW);
  gl::Enable(GL_DEPTH_TEST);
  gl::DepthMask(GL_TRUE);
  gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  m_terrain->update(camera.getPos());

  updateUniforms(m_terrain_program);
  gl::UseProgram(m_terrain_program->getId());

  m_terrain->draw(m_terrain_program);

  CHECK_GL_ERROR();
}

void render_util::viewer::runHeightMapViewer(render_util::Image<float>::ConstPtr height_map,
  util::Factory<TerrainBase> terrain_factory,
  ShaderProgramFactory terrain_program_factory)
{
  auto create_func = [height_map, terrain_factory, terrain_program_factory]
  {
    return make_shared<HeightMapViewerScene>(height_map, terrain_factory, terrain_program_factory);
  };

  runApplication(create_func);

  cout<<"exiting..."<<endl;
}
