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

#ifndef RENDER_UTIL_UTIL_OUTPUT_STREAM_H
#define RENDER_UTIL_UTIL_OUTPUT_STREAM_H


#include <glm/glm.hpp>
#include <ostream>


namespace util
{


class OutputStream
{
  std::ostream &m_out;

public:
  OutputStream(std::ostream &out) : m_out(out) {}

  template <typename T>
  OutputStream &put(const T &arg)
  {
    m_out << arg;
    return *this;
  }

  template <typename T>
  OutputStream &putVec2(const T &arg)
  {
    m_out << '[' << arg.x << ", " << arg.y << ']';
    return *this;
  }

  template <typename T>
  OutputStream &putVec3(const T &arg)
  {
    m_out << '[' << arg.x << ", " << arg.y << ", " << arg.z << ']';
    return *this;
  }

  template <typename T>
  OutputStream &putVec4(const T &arg)
  {
    m_out << '[' << arg.x << ", " << arg.y << ", " << arg.z << ", " << arg.w << ']';
    return *this;
  }

  template <typename T>
  OutputStream &operator<<(const T &arg)
  {
    return put<T>(arg);
  }

  OutputStream &operator<<(const glm::ivec2 &arg)
  {
    return putVec2(arg);
  }

  OutputStream &operator<<(const glm::ivec3 &arg)
  {
    return putVec3(arg);
  }

  OutputStream &operator<<(const glm::ivec4 &arg)
  {
    return putVec4(arg);
  }

  OutputStream &operator<<(const glm::vec2 &arg)
  {
    return putVec2(arg);
  }

  OutputStream &operator<<(const glm::vec3 &arg)
  {
    return putVec3(arg);
  }

  OutputStream &operator<<(const glm::vec4 &arg)
  {
    return putVec4(arg);
  }

  OutputStream &operator<<(const glm::dvec2 &arg)
  {
    return putVec2(arg);
  }

  OutputStream &operator<<(const glm::dvec3 &arg)
  {
    return putVec3(arg);
  }

  OutputStream &operator<<(const glm::dvec4 &arg)
  {
    return putVec4(arg);
  }

};


} // namespace util

#endif
