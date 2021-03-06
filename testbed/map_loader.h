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

#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <render_util/map_loader_base.h>

class MapLoader : public render_util::MapLoaderBase
{
public:
  void loadMap(render_util::Map &map,
               bool load_terrain = true,
               render_util::ElevationMap::Ptr *elevation_map = nullptr,
               render_util::ElevationMap::Ptr *elevation_map_base = nullptr) override;
};

#endif
