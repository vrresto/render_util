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

#ifndef render_util_land_textures_h
#define render_util_land_textures_h

#include <render_util/terrain_base.h>

namespace render_util
{


struct LandTextures
{
  TerrainBase::TypeMap::ConstPtr type_map;
  TerrainBase::TypeMap::ConstPtr base_type_map;
  std::vector<ImageRGBA::Ptr> textures;
  std::vector<ImageRGB::Ptr> textures_nm;
  std::vector<float> texture_scale;
  ImageRGBA::ConstPtr far_texture;
};


}

#endif
