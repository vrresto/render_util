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

#ifndef RENDER_UTIL_VIEWER_H
#define RENDER_UTIL_VIEWER_H

#include <render_util/map_base.h>
#include <render_util/image.h>
#include <render_util/terrain_base.h>
#include <render_util/shader.h>
#include <render_util/texture_manager.h>
#include <factory.h>

#include <functional>

namespace render_util::viewer
{
  struct ElevationMapLoaderBase
  {
    virtual ElevationMap::Ptr createElevationMap() const = 0;
    virtual int getMetersPerPixel() const = 0;

    virtual ElevationMap::Ptr createBaseElevationMap() const = 0;
    virtual int getBaseElevationMapMetersPerPixel() const = 0;
    virtual glm::vec3 getBaseElevationMapOrigin(const glm::vec3 &default_value) const = 0;
    virtual void saveBaseElevationMapOrigin(const glm::vec3&) = 0;
  };


  class MapLoaderBase
  {
  public:
    virtual ~MapLoaderBase() {}

    virtual ElevationMap::Ptr createElevationMap() const = 0;
    virtual void createLandTextures(TerrainBase::Textures&) const = 0;
    virtual void createMapTextures(render_util::MapBase*p, TerrainBase::Textures&) const = 0;
    virtual int getHeightMapMetersPerPixel() const = 0;

    virtual ElevationMap::Ptr createBaseElevationMap() const = 0;
    virtual glm::vec3 getBaseMapOrigin() const = 0;
    virtual unsigned int getBaseElevationMapMetersPerPixel() const = 0;

    virtual std::shared_ptr<const render_util::TerrainBase::TypeMap> getBaseTypeMap() const = 0;
    virtual void generateBaseTypeMap(render_util::ElevationMap::ConstPtr map) = 0;
  };


  using CreateMapLoaderFunc =
    std::function<std::shared_ptr<MapLoaderBase>(const render_util::TextureManager&)>;

  using CreateElevationMapLoaderFunc =
    std::function<std::shared_ptr<ElevationMapLoaderBase>()>;


  void runViewer(CreateMapLoaderFunc&, std::string app_name);
  void runSimpleViewer(CreateElevationMapLoaderFunc&, std::string app_name);

  void runHeightMapViewer(Image<float>::ConstPtr height_map);
}

#endif
