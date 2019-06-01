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
#include <render_util/physics.h>

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

namespace render_util
{


void setCameraUniforms(render_util::ShaderProgramPtr program,
    const std::string &prefix,
    const render_util::Camera &camera)
{
  program->setUniform(prefix + "cameraPosWorld", camera.getPos());
  program->setUniform(prefix + "projectionMatrixFar", camera.getProjectionMatrixFar());
  program->setUniform(prefix + "world2ViewMatrix", camera.getWorld2ViewMatrix());
  program->setUniform(prefix + "view2WorldMatrix", camera.getView2WorldMatrix());
  program->setUniform(prefix + "view_to_world_rotation", camera.getViewToWorldRotation());
  program->setUniform(prefix + "world_to_view_rotation", camera.getWorldToViewRotation());

  program->setUniform<float>(prefix + "fov", camera.getFov());
  program->setUniform<float>(prefix + "z_near", camera.getZNear());
  program->setUniform<float>(prefix + "z_far", camera.getZFar());
}


}

#ifdef RENDER_UTIL_USE_MSVCRT

#include <direct.h>

bool util::mkdir(const char *name)
{
  auto res = _mkdir(name);
  return res == 0 || (res == -1 && errno == EEXIST);
}

bool util::fileExists(std::string path)
{
  struct _stat buffer {};

  auto res = _stat(path.c_str(), &buffer);
  auto error_code = errno;

  if (res == -1 && error_code == ENOENT)
  {
    return false;
  }
  else if (res == -1)
  {
    throw std::runtime_error("_stat() returned error: " + std::to_string(error_code));
  }
  else if (res == 0)
  {
    return true;
  }
  else
  {
    throw std::runtime_error("unexpected return value from _stat(): " + res);
  }
}

#else

bool util::mkdir(const char *name)
{
  auto res = ::mkdir(name, S_IRWXU);
  return res == 0 || (res == -1 && errno == EEXIST);
}

#endif


std::string util::makeTimeStampString()
{
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");

  return oss.str();
}


void render_util::updateUniforms(render_util::ShaderProgramPtr program,
                                 const render_util::Camera &camera,
                                 const std::optional<Camera> prev_camera)
{
  using Vec3 = render_util::Camera::Vec3;

  if (prev_camera)
    setCameraUniforms(program, "prev_", *prev_camera);

  glm::mat4 mvp(camera.getProjectionMatrixFarD() * camera.getWorld2ViewMatrixD());

  program->setUniform("cameraPosWorld", camera.getPos());
  program->setUniform("projectionMatrixFar", camera.getProjectionMatrixFar());
  program->setUniform("world2ViewMatrix", camera.getWorld2ViewMatrix());
  program->setUniform("view2WorldMatrix", camera.getView2WorldMatrix());
  program->setUniform("world_to_view_rotation", camera.getWorldToViewRotation());

  program->setUniform("ndc_to_view", camera.getNDCToViewMatrix());
  program->setUniform("ndc_xy_to_view", camera.getNDCToView());

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

  auto earth_center =
    glm::vec3(camera.getPos().x, camera.getPos().y, -physics::EARTH_RADIUS);

  auto earth_center_view = glm::vec3(camera.getWorld2ViewMatrix() * glm::vec4(earth_center, 1));

  program->setUniform("earth_center", earth_center);
  program->setUniform("earth_center_view", earth_center_view);
  program->setUniform<float>("earth_radius", physics::EARTH_RADIUS);

  program->setUniform("sun_size", glm::vec2(tan(physics::SUN_ANGULAR_RADIUS),
                                            cos(physics::SUN_ANGULAR_RADIUS )));
}
