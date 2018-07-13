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

#ifndef ENGINE_ENGINE_H
#define ENGINE_ENGINE_H

#include <string>
#include <glm/glm.hpp>

namespace engine 
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

  const std::string &getResourcePath();
  const std::string &getDataPath();
}

#endif
