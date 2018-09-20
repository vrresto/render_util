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

#ifndef RENDER_UTIL_CAMERA_H
#define RENDER_UTIL_CAMERA_H

#include <render_util/render_util.h>

#include <glm/glm.hpp>

namespace render_util
{
  class Camera
  {
    struct Private;
    Private *p = 0;

  public:
    Camera();
    const glm::mat4 &getView2WorldMatrix() const;
    const glm::mat4 &getWorld2ViewMatrix() const;
    const glm::mat4 &getProjectionMatrixFar() const;
    const glm::vec3 &getPos() const;
    const glm::ivec2 &getViewportSize() const;
    bool cull(const Box &box) const;
    float getFov() const;

    void setTransform(float x, float y, float z, float yaw, float pitch, float roll);
    void setViewportSize(int width, int height);
    void setFov(float fov);
    void setProjection(float fov, float z_near, float z_far);
  };

}

#endif
