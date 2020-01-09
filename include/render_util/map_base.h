/**
 *    Rendering utilities
 *    Copyright (C) 2019 Jan Lepper
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

#ifndef RENDER_UTIL_MAP_BASE_H
#define RENDER_UTIL_MAP_BASE_H

#include <render_util/map_textures.h>
#include <render_util/water.h>

namespace render_util
{


struct MapBase
{
  virtual ~MapBase() {}

  virtual MapTextures &getTextures() = 0;
  virtual WaterAnimation &getWaterAnimation() = 0;
};


}

#endif
