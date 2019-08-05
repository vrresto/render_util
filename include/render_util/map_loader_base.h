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

#ifndef RENDER_UTIL_MAPLOADER_BASE_H
#define RENDER_UTIL_MAPLOADER_BASE_H

#include <render_util/map.h>
#include <render_util/terrain_base.h>

#include <string>

namespace render_util
{
  struct MapBase
  {
    virtual ~MapBase() {}

    virtual MapTextures &getTextures() = 0;
    virtual WaterAnimation &getWaterAnimation() = 0;
    virtual void setMaterialMap(TerrainBase::MaterialMap::ConstPtr) = 0;
    virtual void setCirrusTexture(std::shared_ptr<GenericImage>) {};

  };


  class MapLoaderBase
  {
  public:
    struct TerrainTextures
    {
      TerrainBase::TypeMap::ConstPtr type_map;
      std::vector<ImageRGBA::Ptr> textures;
      std::vector<ImageRGB::Ptr> textures_nm;
      std::vector<float> texture_scale;
      ImageRGBA::ConstPtr far_texture;
    };

    virtual ~MapLoaderBase() {}

    virtual void createMapTextures(MapBase*) const = 0;
    virtual ElevationMap::Ptr createElevationMap() const = 0;
    virtual void createTerrainTextures(TerrainTextures&) const = 0;
    virtual int getHeightMapMetersPerPixel() const = 0;

    virtual ElevationMap::Ptr createBaseElevationMap(ImageGreyScale::ConstPtr land_map) const = 0;
    virtual ImageGreyScale::Ptr createBaseLandMap() const = 0;
    virtual glm::vec2 getBaseMapOrigin() const { return {}; };
  };
}

#endif
