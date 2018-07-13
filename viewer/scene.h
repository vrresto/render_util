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

#include <render_util/shader.h>

class Scene
{
public:
  Camera camera;
  float sun_azimuth = 90.0;
  bool toggle_lod_morph = false;
  bool pause_animations = false;

  glm::vec3 getSunDir()
  {
    glm::vec2 sun_dir_h = glm::vec2(glm::cos(glm::radians(sun_azimuth)), glm::sin(glm::radians(sun_azimuth)));

    return glm::vec3(0, sun_dir_h.x, sun_dir_h.y);
  }

  virtual void updateUniforms(engine::ShaderProgramPtr program)
  {
    program->setUniform("cameraPosWorld", camera.getPos());
    program->setUniform("projectionMatrixFar", camera.getProjectionMatrixFar());
    program->setUniform("world2ViewMatrix", camera.getWorld2ViewMatrix());
    program->setUniform("view2WorldMatrix", camera.getView2WorldMatrix());
    program->setUniform("sunDir", getSunDir());
    program->setUniform("toggle_lod_morph", toggle_lod_morph);
  }

  virtual void setup() = 0;
  virtual void render(float frame_delta) = 0;
};

#endif
