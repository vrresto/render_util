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


#define GLM_ENABLE_EXPERIMENTAL

#include <render_util/render_util.h>
#include <render_util/camera.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <iostream>
#include <vector>


using namespace glm;
using namespace render_util;

using Mat4 = Camera::Mat4;
using Vec3 = Camera::Vec3;
using Unit = Camera::Unit;


namespace render_util
{

  struct Camera::Private
  {
    Mat4 world_to_view;
    Mat4 view_to_world;
    Mat4 projection_far;
    Mat4 world_to_view_rotation;
    Mat4 view_to_world_rot;

    Vec3 pos;
    ivec2 viewport_size;

    vec2 ndc_to_view = vec2(1);

    Unit m_z_near = 1.2;
    Unit m_z_far = 2300000;
    Unit m_fov = 90;

    std::vector<Plane> frustum_planes;

    Private();
    Private(const Private &other);
    void calcFrustumPlanes();
    void refreshProjection();
    Vec3 view2World(Vec3 p) const;
  };


  Camera::Private::Private()
  {
    refreshProjection();
  }

  Camera::Private::Private(const Private &other)
  {
    *this = other;
  }

  void Camera::Private::refreshProjection()
  {
//     const float z_near = 1.2f;
//     const float z_far = 44800.2f;

//     const float z_far = 44800.2f;
//     const float z_far = 300000;
//     const float z_far = 2300000;


//     const float z_near = 100.2f;
//     const float z_far = 448000.2f;

  //   const float z_far = 48000.2f;
//     const float z_near = 100.2f;
//     const float z_far = 3500.0 * 1000.0;
//     const float z_far = 35000.0 * 1000.0;

    const auto aspect = (Unit)viewport_size.x / (Unit)viewport_size.y;

    const auto right = m_z_near * tan(radians(m_fov) / (Unit)2.0);
    const auto top = right / aspect;
    projection_far = frustum(-right, right, -top, top, m_z_near, m_z_far);

    calcFrustumPlanes();

    ndc_to_view.x = tan(radians(m_fov)/2.f);
    ndc_to_view.y = ndc_to_view.x / aspect;
  }


  Vec3 Camera::Private::view2World(Vec3 p) const
  {
    p.z *= -1;
    return xyz(view_to_world * vec4(p, 1));
  }


  void Camera::Private::calcFrustumPlanes()
  {
    frustum_planes.clear();

    const auto aspect = (Unit)viewport_size.x / (Unit)viewport_size.y;

    auto right = m_z_near * tan(radians(m_fov)/2.0);
    auto left = -right;
    auto top = right / aspect;
    auto bottom = -top;

    vec3 top_left = view2World(vec3(left, top, m_z_near));
    vec3 bottom_left = view2World(vec3(left, bottom, m_z_near));
    vec3 top_right = view2World(vec3(right, top, m_z_near));
    vec3 bottom_right = view2World(vec3(right, bottom, m_z_near));

    Plane left_plane(top_left, bottom_left, pos, true);
    Plane right_plane(top_right, bottom_right, pos, false);
    Plane top_plane(top_left, top_right, pos, false);
    Plane bottom_plane(bottom_left, bottom_right, pos, true);

    Plane near_plane(bottom_left, bottom_right, top_right, true);

    Plane far_plane = near_plane;
    far_plane.move(m_z_far - m_z_near);
    far_plane.flip();

    frustum_planes.push_back(left_plane);
    frustum_planes.push_back(right_plane);
    frustum_planes.push_back(top_plane);
    frustum_planes.push_back(bottom_plane);
    frustum_planes.push_back(near_plane);
    frustum_planes.push_back(far_plane);
  }


  Camera::Camera() : p(std::make_unique<Private>()) {}

  Camera::Camera(const Camera &other) : p(std::make_unique<Private>(*other.p)) {}

  Camera &Camera::operator=(const Camera &other)
  {
    *p = *other.p;
    return *this;
  }

  Camera::~Camera() {}

  const Mat4 &Camera::getWorldToViewRotationD() const { return p->world_to_view_rotation; }
  const Mat4 &Camera::getProjectionMatrixFarD() const { return p->projection_far; }
  const Mat4 &Camera::getWorld2ViewMatrixD() const { return p->world_to_view; }
  const Mat4 &Camera::getView2WorldMatrixD() const { return p->view_to_world; }
  const Vec3 &Camera::getPosD() const { return p->pos; }
  const ivec2 &Camera::getViewportSize() const { return p->viewport_size; }
  const vec2 &Camera::getNDCToView() const { return p->ndc_to_view; }


  void Camera::setViewportSize(int width, int height) {
    p->viewport_size.x = width;
    p->viewport_size.y = height;
    p->refreshProjection();
  }


  void Camera::setFov(Unit fov)
  {
    p->m_fov = fov;
    p->refreshProjection();
  }


  Unit Camera::getFov() const
  {
    return p->m_fov;
  }


  Unit Camera::getZNear() const
  {
    return p->m_z_near;
  }


  Unit Camera::getZFar() const
  {
    return p->m_z_far;
  }


  void Camera::setProjection(Unit fov, Unit z_near, Unit z_far)
  {
    p->m_fov = fov;
    p->m_z_near = z_near;
    p->m_z_far = z_far;
    p->refreshProjection();
  }


  void Camera::setTransform(Unit x, Unit y, Unit z, Unit yaw, Unit pitch, Unit roll)
  {
    Vec3 pos(x,y,z);
    p->pos = pos;

    Mat4 world_to_y_up(1);
    world_to_y_up = rotate(world_to_y_up, radians((Unit)90.0),
                          Vec3(0.0, 1.0, 0.0));
    world_to_y_up = rotate(world_to_y_up, radians((Unit)-90.0),
                          Vec3(1.0, 0.0, 0.0));

    Mat4 world_to_view_trans = translate(Mat4(1), -pos);
    Mat4 world_to_view_pitch = rotate(Mat4(1), radians(-pitch),
                                      Vec3(1.0, 0.0, 0.0));
    Mat4 world_to_view_yaw = rotate(Mat4(1), radians(-yaw),
                                    Vec3(0.0, 1.0, 0.0));
    Mat4 world_to_view_roll = rotate(Mat4(1), radians(-roll),
                                      Vec3(0.0, 0.0, 1.0));

    p->world_to_view =
      world_to_view_roll *
      world_to_view_pitch *
      world_to_view_yaw * 
      world_to_y_up *
      world_to_view_trans;

    p->view_to_world = affineInverse(p->world_to_view);

    p->world_to_view_rotation =
      world_to_view_roll *
      world_to_view_pitch *
      world_to_view_yaw *
      world_to_y_up;

    p->view_to_world_rot = affineInverse(p->world_to_view_rotation);

    p->calcFrustumPlanes();
  }


  bool Camera::cull(const Box &box) const
  {
    for (auto &plane : p->frustum_planes)
    {
      if (plane.cull(box))
        return true;
    }
    return false;
  }


  Beam Camera::createBeamThroughViewportCoord(const vec2 &coord) const
  {
    auto rel_coord = clamp(vec2(coord) / vec2(p->viewport_size), vec2(0), vec2(1));
    rel_coord.x = 1 - rel_coord.x;

    auto ndc = 2.f * rel_coord - vec2(1);
    auto view = ndc * p->ndc_to_view;

    auto beam_dir_view = normalize(vec3(view.x, view.y, 1));
    auto beam_dir_world = vec3(p->view_to_world_rot * vec4(beam_dir_view, 0));

    Beam beam;
    beam.origin = p->pos;
    beam.direction = beam_dir_world;

    return beam;
  }

}
