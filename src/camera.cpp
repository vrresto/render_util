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
#include <vector>

using namespace glm;
using namespace render_util;


namespace
{


struct Plane
{
  vec3 point;
  vec3 normal;

  Plane(const vec3 &p1, const vec3 &p2, const vec3 &p3, bool flip_)
  {
    vec3 points[3] { p1, p2, p3 };
    point = p3;
    normal = render_util::calcNormal(points);
    if (flip_)
      flip();
  }


  void flip()
  {
    normal *= -1;
  }


  void move(float dist)
  {
    point += normal * dist;
  }


  float distance(const vec3 &pos)
  {
    return dot(normal, pos - point);
  }

  bool cull(const Box &box)
  {
    for (auto &c : box.getCornerPoints())
    {
      if (distance(c) >= 0)
        return false;
    }
    return true;
  }
};


} // namespace


namespace render_util
{

  struct Camera::Private
  {
    mat4 world_to_view;
    mat4 view_to_world;
    mat4 projection_far;

    vec3 pos;
    ivec2 viewport_size;

    float m_z_near = 1.2;
    float m_z_far = 2300000;
    float m_fov = 90;

    std::vector<Plane> frustum_planes;

    Private();
    Private(const Private &other);
    void calcFrustumPlanes();
    void refreshProjection();
    vec3 view2World(vec3 p) const;
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

    const float aspect = (float)viewport_size.x / (float)viewport_size.y;

    const float right = m_z_near * tan(radians(m_fov)/2.f);
    const float top = right / aspect;
    projection_far = frustum(-right, right, -top, top, m_z_near, m_z_far);

    calcFrustumPlanes();
  }


  vec3 Camera::Private::view2World(vec3 p) const
  {
    p.z *= -1;
    return xyz(view_to_world * vec4(p, 1));
  }


  void Camera::Private::calcFrustumPlanes()
  {
    frustum_planes.clear();

    const float aspect = (float)viewport_size.x / (float)viewport_size.y;

    float right = m_z_near * tan(radians(m_fov)/2.f);
    float left = -right;
    float top = right / aspect;
    float bottom = -top;

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


  Camera::Camera() : p(new Private) {}

  Camera::Camera(const Camera &other) : p(new Private(*other.p)) {}


  const mat4 &Camera::getProjectionMatrixFar() const { return p->projection_far; }
  const mat4 &Camera::getWorld2ViewMatrix() const { return p->world_to_view; }
  const mat4 &Camera::getView2WorldMatrix() const { return p->view_to_world; }
  const vec3 &Camera::getPos() const { return p->pos; }
  const ivec2 &Camera::getViewportSize() const { return p->viewport_size; }


  void Camera::setViewportSize(int width, int height) {
    p->viewport_size.x = width;
    p->viewport_size.y = height;
    p->refreshProjection();
  }


  void Camera::setFov(float fov)
  {
    p->m_fov = fov;
    p->refreshProjection();
  }


  float Camera::getFov() const
  {
    return p->m_fov;
  }


  float Camera::getZNear() const
  {
    return p->m_z_near;
  }


  float Camera::getZFar() const
  {
    return p->m_z_far;
  }


  void Camera::setProjection(float fov, float z_near, float z_far)
  {
    p->m_fov = fov;
    p->m_z_near = z_near;
    p->m_z_far = z_far;
    p->refreshProjection();
  }


  void Camera::setTransform(float x, float y, float z, float yaw, float pitch, float roll)
  {
    vec3 pos(x,y,z);
    p->pos = pos;

    mat4 world_to_y_up(1);
    world_to_y_up = rotate(world_to_y_up, radians(90.f),
                          vec3(0.0f, 1.0f, 0.0f));
    world_to_y_up = rotate(world_to_y_up, radians(-90.f),
                          vec3(1.0f, 0.0f, 0.0f));
  
    mat4 world_to_view_trans = translate(mat4(1), -pos);
    mat4 world_to_view_pitch = rotate(mat4(1), radians(-pitch),
                                      vec3(1.0f, 0.0f, 0.0f));
    mat4 world_to_view_yaw = rotate(mat4(1), radians(-yaw),
                                    vec3(0.0f, 1.0f, 0.0f));
    mat4 world_to_view_roll = rotate(mat4(1), radians(-roll),
                                      vec3(0.0f, 0.0f, 1.0f));

    p->world_to_view =
      world_to_view_roll *
      world_to_view_pitch *
      world_to_view_yaw * 
      world_to_y_up *
      world_to_view_trans;

    p->view_to_world = affineInverse(p->world_to_view);
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

}
