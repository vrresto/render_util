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

#ifndef RENDER_UTIL_GEOMETRY_H
#define RENDER_UTIL_GEOMETRY_H

#include <string>
#include <array>
#include <glm/glm.hpp>

namespace render_util
{

  struct Float3
  {
    float x = 0;
    float y = 0;
    float z = 0;

    glm::vec3 to_vec3() const
    {
      return glm::vec3(x, y, z);
    }
  };

  typedef Float3 Normal;


  inline glm::vec3 calcNormal(glm::vec3 vertices[3])
  {
    glm::vec3 a = vertices[0] - vertices[1];
    glm::vec3 b = vertices[0] - vertices[2];

    return glm::normalize(glm::cross(a,b));
  }


  class Box
  {
    glm::vec3 m_origin;
    glm::vec3 m_extent;
    std::array<glm::vec3, 8> m_corner_points;

    void updateCornerPoints()
    {
      auto it = m_corner_points.begin();

      *(it++) = m_origin + (m_extent * glm::vec3(0, 0, 0));
      *(it++) = m_origin + (m_extent * glm::vec3(0, 0, 1));
      *(it++) = m_origin + (m_extent * glm::vec3(0, 1, 0));
      *(it++) = m_origin + (m_extent * glm::vec3(0, 1, 1));
      *(it++) = m_origin + (m_extent * glm::vec3(1, 0, 0));
      *(it++) = m_origin + (m_extent * glm::vec3(1, 0, 1));
      *(it++) = m_origin + (m_extent * glm::vec3(1, 1, 0));
      *(it++) = m_origin + (m_extent * glm::vec3(1, 1, 1));

      assert(it == m_corner_points.end());
    }

  public:
    const std::array<glm::vec3, 8> &getCornerPoints() const { return m_corner_points; }

    void set(const glm::vec3 &origin, const glm::vec3 &extent)
    {
      m_origin = origin;
      m_extent = extent;
      updateCornerPoints();
    }
  };


  struct Plane
  {
    using vec3 = glm::vec3;

    vec3 point;
    vec3 normal;

    Plane(const vec3 &point, const vec3 &normal) : point(point), normal(normal) {}

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

}

#endif
