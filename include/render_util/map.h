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

#ifndef ENGINE_MAP_H
#define ENGINE_MAP_H

// #include <render_util/image.h>
// #include <render_util/elevation_map.h>
#include <render_util/map_textures.h>
#include <render_util/terrain_base.h>

// #include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace render_util
{
  struct Map
  {
    glm::ivec2 size = glm::ivec2(0);
    glm::ivec2 type_map_size = glm::ivec2(0);
    std::shared_ptr<MapTextures> textures;
    std::shared_ptr<TerrainBase> terrain;
  };

}

#endif
