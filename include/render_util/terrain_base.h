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

#ifndef ENGINE_TERRAIN_BASE_H
#define ENGINE_TERRAIN_BASE_H

#include <render_util/shader.h>

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace render_util
{
  class ElevationMap;
  class TextureManager;

  class TerrainBase
  {
  public:
    virtual ~TerrainBase() {}

    virtual const std::string &getName() = 0;
    virtual void build(const ElevationMap *map) = 0;
    virtual void draw(ShaderProgramPtr program) = 0;
    virtual void update(glm::vec3 camera_pos) {}
    virtual void setTextureManager(TextureManager*) {};
    virtual void setDrawDistance(float dist) {}

    virtual std::vector<glm::vec3> getNormals() { return std::vector<glm::vec3>(); }
  };

}

#endif
