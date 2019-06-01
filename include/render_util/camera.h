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

#include <render_util/geometry.h>


namespace render_util
{
  struct Beam
  {
    glm::vec3 origin;
    glm::vec3 direction;
  };

  class Camera
  {
    struct Private;
    Private *p = 0;

  public:
    using Mat4 = glm::dmat4;
    using Vec3 = glm::dvec3;
    using Unit = double;


    Camera();
    Camera(const Camera &other);

    const Mat4 &getWorldToViewRotationD() const;
    const Mat4 &getViewToWorldRotationD() const;
    const Mat4 &getView2WorldMatrixD() const;
    const Mat4 &getWorld2ViewMatrixD() const;
    const Mat4 &getProjectionMatrixFarD() const;
    const Vec3 &getPosD() const;
    const glm::ivec2 &getViewportSize() const;
    bool cull(const Box &box) const;
    Unit getFov() const;
    Unit getZNear() const;
    Unit getZFar() const;


    glm::mat4 getWorldToViewRotation() const
    {
      return glm::mat4(getWorldToViewRotationD());
    }

    glm::mat4 getViewToWorldRotation() const
    {
      return glm::mat4(getViewToWorldRotationD());
    }

    Mat4 getVP() const
    {
      return getProjectionMatrixFarD() * getWorld2ViewMatrixD();
    }

    glm::mat4 getVP_s() const
    {
      return glm::mat4(getVP());
    }

    const glm::mat4 getView2WorldMatrix() const
    {
      return glm::mat4(getView2WorldMatrixD());
    }

    const glm::mat4 getWorld2ViewMatrix() const
    {
      return glm::mat4(getWorld2ViewMatrixD());
    }

    const glm::mat4 getProjectionMatrixFar() const
    {
      return glm::mat4(getProjectionMatrixFarD());
    }

    const glm::vec3 getPos() const
    {
      return glm::vec3(getPosD());
    }

    Beam createBeamThroughViewportCoord(const glm::vec2&) const;

    void setTransform(Unit x, Unit y, Unit z, Unit yaw, Unit pitch, Unit roll);
    void setViewportSize(int width, int height);
    void setFov(Unit fov);
    void setProjection(Unit fov, Unit z_near, Unit z_far);
  };

}

#endif
