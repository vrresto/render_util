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

#ifndef SCENE_H
#define SCENE_H

#include "camera.h"
#include <render_util/viewer_parameter.h>
#include <render_util/render_util.h>
#include <render_util/shader.h>
#include <render_util/terrain_util.h>
#include <render_util/atmosphere.h>
#include <render_util/globals.h>
#include <render_util/gl_binding/gl_functions.h>


namespace render_util::viewer
{

using namespace gl_binding;


struct Terrain
{
  std::shared_ptr<TerrainBase> m_terrain;
  std::shared_ptr<TerrainBase> getTerrain() { return m_terrain; }

  void draw(const Camera &camera, TerrainBase::Client *client)
  {
    gl::FrontFace(GL_CCW);
    gl::Enable(GL_DEPTH_TEST);
    gl::DepthMask(GL_TRUE);

    getTerrain()->setDrawDistance(0);
    getTerrain()->update(camera, false);

//     gl::PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    getTerrain()->draw(client);
//     gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    CHECK_GL_ERROR();
  }
};


class Scene
{
  render_util::TextureManager texture_manager = render_util::TextureManager(0);

public:
  virtual ~Scene() {}

  const bool m_use_base_map = false;
  const bool m_use_base_water_map = false;

#if ENABLE_BASE_MAP
  glm::vec2 base_map_origin = glm::vec2(0);
  glm::vec2 base_map_size_m = glm::vec2(0);
#endif

  Camera camera;
  float sun_elevation = 90.0;
  float sun_azimuth = 0.0;
  bool toggle_lod_morph = false;
  bool pause_animations = false;

  Parameters m_parameters;

  Terrain m_terrain;
  std::unique_ptr<Atmosphere> m_atmosphere;

  void addAtmosphereController(std::string name, Atmosphere::Parameter p)
  {
    if (m_atmosphere->hasParameter(p))
    {
      using Wrapper = ValueWrapper<float>;

      Wrapper wrapper =
      {
        [this,p] { return m_atmosphere->getParameter(p); },
        [this,p] (auto value) { m_atmosphere->setParameter(p, value); },
      };
      m_parameters.add<float>(name, wrapper);
    }
  }

  void createControllers()
  {
    addAtmosphereController("exposure", Atmosphere::Parameter::EXPOSURE);
    addAtmosphereController("saturation", Atmosphere::Parameter::SATURATION);
    addAtmosphereController("brightness_curve_exponent", Atmosphere::Parameter::BRIGHTNESS_CURVE_EXPONENT);
    addAtmosphereController("texture_brightness", Atmosphere::Parameter::TEXTURE_BRIGHTNESS);
    addAtmosphereController("texture_brightness_curve_exponent",
                            Atmosphere::Parameter::TEXTURE_BRIGHTNESS_CURVE_EXPONENT);
    addAtmosphereController("texture_saturation", Atmosphere::Parameter::TEXTURE_SATURATION);
    addAtmosphereController("gamma", Atmosphere::Parameter::GAMMA);
    addAtmosphereController("blue saturation", Atmosphere::Parameter::BLUE_SATURATION);

    addAtmosphereController("uncharted2_a", Atmosphere::Parameter::UNCHARTED2_A);
    addAtmosphereController("uncharted2_b", Atmosphere::Parameter::UNCHARTED2_B);
    addAtmosphereController("uncharted2_c", Atmosphere::Parameter::UNCHARTED2_C);
    addAtmosphereController("uncharted2_d", Atmosphere::Parameter::UNCHARTED2_D);
    addAtmosphereController("uncharted2_e", Atmosphere::Parameter::UNCHARTED2_E);
    addAtmosphereController("uncharted2_f", Atmosphere::Parameter::UNCHARTED2_F);
    addAtmosphereController("uncharted2_w", Atmosphere::Parameter::UNCHARTED2_W);
  }

  bool hasActiveParameter()
  {
    return !m_parameters.getParameters().empty();
  }

  const std::vector<std::unique_ptr<Parameter>> &getParameters()
  {
    return m_parameters.getParameters();
  }
  Parameter &getActiveParameter() { return m_parameters.getActive(); }
  void setActiveParameter(int index) { m_parameters.setActive(index); }
  int getActiveParameterIndex() { return m_parameters.getActiveIndex(); }

  glm::vec3 getSunDir()
  {
    glm::vec4 pitch_axis(1,0,0,0);

    glm::mat4 yaw = glm::rotate(glm::mat4(1), glm::radians(sun_azimuth), glm::vec3(0.0, 0.0, 1.0));

    pitch_axis = yaw * pitch_axis;

    glm::mat4 pitch = glm::rotate(glm::mat4(1), glm::radians(sun_elevation), glm::vec3(pitch_axis));

    auto dir = yaw * glm::vec4(0,1,0,0);
    dir = pitch * dir;

    return glm::vec3(dir);
  }

  void createTerrain(render_util::TerrainBase::BuildParameters &params,
                     const ShaderSearchPath &shader_search_path,
                     glm::vec2 base_map_origin)
  {
    m_terrain.m_terrain = render_util::createTerrain(texture_manager, true, shader_search_path);

//     assert(textures.type_map);
//     assert(!textures.textures.empty());
//     assert(textures.texture_scale.size() == textures.textures.size());

    m_terrain.getTerrain()->setBaseMapOrigin(base_map_origin);
    m_terrain.getTerrain()->build(params);

  }

//   void updateTerrain()
//   {
//     m_terrain.update(camera);
//     m_terrain_cdlod.update(camera);
//   }

  void drawTerrain();

  render_util::TextureManager &getTextureManager() { return texture_manager; }

  virtual void updateUniforms(render_util::ShaderProgramPtr program)
  {
    program->setUniform("sunDir", getSunDir());
    program->setUniform("toggle_lod_morph", toggle_lod_morph);
    render_util::updateUniforms(program, camera);
    m_atmosphere->setUniforms(program);
  }

  virtual void mark() {}
  virtual void unmark() {}
  virtual void cursorPos(const glm::dvec2&) {}
  virtual void rebuild() {}

  virtual void setup() = 0;
  virtual void render(float frame_delta) = 0;
};


class TerrainClient : public TerrainBase::Client
{
  Scene *m_scene = nullptr;

public:
  TerrainClient(Scene *scene) : m_scene(scene) {}

  void setActiveProgram(ShaderProgramPtr p) override
  {
    render_util::getCurrentGLContext()->setCurrentProgram(p);
    m_scene->updateUniforms(p);
  }
};


inline void Scene::drawTerrain()
{
  TerrainClient client(this);
  m_terrain.draw(camera, &client);
}


} // namespace render_util::viewer


#endif
