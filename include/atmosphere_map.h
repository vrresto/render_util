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

#include <glm/glm.hpp>

namespace
{
  typedef float AtmosphereMapElementType;

  enum 
  {
    ATMOSPHERE_MAP_WIDTH = 128,
    ATMOSPHERE_MAP_HEIGHT = 128,
    ATMOSPHERE_MAP_NUM_ELEMENTS = ATMOSPHERE_MAP_WIDTH * ATMOSPHERE_MAP_HEIGHT
  };

  const glm::ivec2
    atmosphere_map_dimensions(ATMOSPHERE_MAP_WIDTH,
                              ATMOSPHERE_MAP_HEIGHT);

  const int atmosphere_map_num_elements =
        atmosphere_map_dimensions.x *
        atmosphere_map_dimensions.y;

  const int atmosphere_map_size_bytes = atmosphere_map_num_elements * sizeof(AtmosphereMapElementType);
}
