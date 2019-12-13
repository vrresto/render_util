/**
 *    Rendering utilities
 *    Copyright (C) 2019  Jan Lepper
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

#ifndef RENDER_UTIL_VIEWER_SCENEBASE_H
#define RENDER_UTIL_VIEWER_SCENEBASE_H

#include "camera.h"

#include <render_util/render_util.h>
#include <render_util/map_loader_base.h>
#include <render_util/shader.h>
#include <render_util/terrain_util.h>
#include <render_util/atmosphere.h>
#include <render_util/globals.h>
#include <render_util/parameter_wrapper.h>
#include <render_util/gl_binding/gl_functions.h>


namespace render_util::viewer
{


class SceneBase
{
  render_util::TextureManager texture_manager = render_util::TextureManager(0);

public:
  class TerrainClient : public TerrainBase::Client
  {
    SceneBase &m_scene;

  public:
    TerrainClient(SceneBase &scene) : m_scene(scene) {}

    void setActiveProgram(ShaderProgramPtr p) override
    {
      render_util::getCurrentGLContext()->setCurrentProgram(p);
      m_scene.updateUniforms(p);
    }
  };

  Camera camera;

  virtual ~SceneBase() {}

  render_util::TextureManager &getTextureManager() { return texture_manager; }

  virtual glm::vec3 getSunDir() { return glm::normalize(glm::vec3(0.5)); }

  virtual void updateUniforms(render_util::ShaderProgramPtr program)
  {
    render_util::updateUniforms(program, camera);
    program->setUniform("sunDir", getSunDir());
    program->setUniform("terrain_height_offset", 0.f);
  }

  virtual void cursorPos(const glm::dvec2&) {}

  virtual void setup() = 0;
  virtual void render(float frame_delta) = 0;
};


} // namespace render_util::viewer


#endif
