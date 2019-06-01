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

#include <util.h>
#include <render_util/render_util.h>
#include <render_util/terrain_base.h>

#ifdef RENDER_UTIL_USE_MSVCRT

#include <direct.h>

bool util::mkdir(const char *name)
{
  auto res = _mkdir(name);
  return res == 0 || (res == -1 && errno == EEXIST);
}

#endif

void render_util::updateUniforms(render_util::ShaderProgramPtr program, const render_util::Camera &camera)
{
  using Vec3 = render_util::Camera::Vec3;

  glm::mat4 mvp(camera.getProjectionMatrixFarD() * camera.getWorld2ViewMatrixD());

  program->setUniform("cameraPosWorld", camera.getPos());
  program->setUniform("projectionMatrixFar", camera.getProjectionMatrixFar());
  program->setUniform("world2ViewMatrix", camera.getWorld2ViewMatrix());
  program->setUniform("view2WorldMatrix", camera.getView2WorldMatrix());
  program->setUniform("world_to_view_rotation", camera.getWorldToViewRotation());
  program->setUniform("view_to_world_rotation", camera.getViewToWorldRotation());
  program->setUniform<glm::vec2>("viewport_size", camera.getViewportSize());
  program->setUniform<float>("fov", camera.getFov());
  program->setUniform<float>("z_near", camera.getZNear());
  program->setUniform<float>("z_far", camera.getZFar());

  Vec3 terrain_scale(glm::ivec2(TerrainBase::GRID_RESOLUTION_M), 1);

  program->setUniform("camera_pos_terrain_floor",
                      glm::vec3(glm::floor(camera.getPosD() / terrain_scale)));
  program->setUniform("camera_pos_offset_terrain",
                      glm::vec3(terrain_scale * glm::fract(camera.getPosD() / terrain_scale)));
}
