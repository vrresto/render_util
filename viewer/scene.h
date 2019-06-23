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

#include <render_util/render_util.h>
#include <render_util/map_loader_base.h>
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
//     gl::PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    getTerrain()->setDrawDistance(0);
    getTerrain()->update(camera, false);

    getTerrain()->draw(client);

//     gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    CHECK_GL_ERROR();
  }
};


class Scene
{
  render_util::TextureManager texture_manager = render_util::TextureManager(0);

public:
  struct Controller
  {
    using SetFunc = std::function<void(float)>;
    using GetFunc = std::function<float()>;

    SetFunc set;
    GetFunc get;

    std::string name;
    float default_value = 0;

    Controller(std::string name_, SetFunc set_, GetFunc get_) : name(name_), set(set_), get(get_)
    {
      default_value = get();
    }

    void reset()
    {
      set(default_value);
    }
  };


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
  int m_active_controller = 0;
  std::vector<Controller> m_controllers;

  Terrain m_terrain;
  std::unique_ptr<Atmosphere> m_atmosphere;


  void addController(std::string name, Controller::SetFunc set, Controller::GetFunc get)
  {
    m_controllers.push_back(Controller(name, set, get));
  }


  void addAtmosphereController(std::string name, Atmosphere::Parameter p)
  {
    if (m_atmosphere->hasParameter(p))
    {
      addController(name,
                    [this,p] (auto value) { m_atmosphere->setParameter(p, value); },
                    [this,p] { return m_atmosphere->getParameter(p); });
    }
  }

  void createControllers()
  {
    addAtmosphereController("exposure", Atmosphere::Parameter::EXPOSURE);
    addAtmosphereController("saturation", Atmosphere::Parameter::SATURATION);
    addAtmosphereController("texture_brightness", Atmosphere::Parameter::TEXTURE_BRIGHTNESS);
    addAtmosphereController("texture_brightness_curve_exponent",
                            Atmosphere::Parameter::TEXTURE_BRIGHTNESS_CURVE_EXPONENT);
    addAtmosphereController("texture_saturation", Atmosphere::Parameter::TEXTURE_SATURATION);
    addAtmosphereController("gamma", Atmosphere::Parameter::GAMMA);
  }


  bool hasActiveController()
  {
    return m_active_controller >= 0 && m_active_controller < m_controllers.size();
  }

  Controller &getActiveController() { return m_controllers.at(m_active_controller); }

  void setActiveController(int index)
  {
    if (index < 0)
      index += m_controllers.size();
    index = index % m_controllers.size();
    m_active_controller = index;
  }


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

  void createTerrain(render_util::ElevationMap::ConstPtr elevation_map,
                     render_util::TerrainBase::MaterialMap::ConstPtr material_map,
                     render_util::MapLoaderBase::TerrainTextures &textures,
                     const ShaderSearchPath &shader_search_path,
                     const ShaderParameters &shader_params)
  {
    m_terrain.m_terrain = render_util::createTerrain(texture_manager, true, shader_search_path);

    assert(textures.type_map);
    assert(!textures.textures.empty());
    assert(textures.texture_scale.size() == textures.textures.size());

    m_terrain.getTerrain()->build(elevation_map,
                          material_map,
                          textures.type_map,
                          textures.textures,
                          textures.textures_nm,
                          textures.texture_scale,
                          shader_params);

  }

  void updateTerrain(const render_util::ElevationMap::ConstPtr elevation_map_base)
  {
    m_terrain.getTerrain()->setBaseElevationMap(elevation_map_base);
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
#if ENABLE_BASE_MAP
    program->setUniform("height_map_base_origin", base_map_origin);
#endif
    render_util::updateUniforms(program, camera);
    m_atmosphere->setUniforms(program, camera);
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
