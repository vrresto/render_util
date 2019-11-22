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

#ifndef RENDER_UTIL_TERRAIN_BASE_H
#define RENDER_UTIL_TERRAIN_BASE_H

#include <render_util/shader.h>
#include <render_util/camera.h>
#include <render_util/elevation_map.h>
#include <render_util/texture_manager.h>
#include <factory.h>

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace render_util
{
  enum { HEIGHT_MAP_BASE_METERS_PER_PIXEL = 400 };

  class TerrainBase
  {
  public:
    static constexpr int TILE_SIZE_M = 1600;
    static constexpr int GRID_RESOLUTION_M = 200;

    struct MaterialID
    {
      static constexpr unsigned int LAND = 1;
      static constexpr unsigned int WATER = 1 << 1;
      static constexpr unsigned int FOREST = 1 << 2;
      static constexpr unsigned int ALL = LAND | WATER | FOREST;
    };

    using MaterialMap = Image<unsigned char>;
    using TypeMap = ImageGreyScale;

    struct Client
    {
      virtual void setActiveProgram(ShaderProgramPtr) = 0;
    };

    virtual ~TerrainBase() {}

    virtual void build(ElevationMap::ConstPtr map,
                       MaterialMap::ConstPtr,
                       TypeMap::ConstPtr,
                       std::vector<ImageRGBA::Ptr> &textures,
                       std::vector<ImageRGB::Ptr> &textures_nm,
                       const std::vector<float> &texture_scale,
                       const ShaderParameters&) = 0;

    virtual void draw(Client *client = nullptr) = 0;
    virtual void setBaseElevationMap(ElevationMap::ConstPtr map) {}
    virtual void update(const Camera &camera, bool low_detail) {}
    virtual void setDrawDistance(float dist) {}
    virtual std::vector<glm::vec3> getNormals() { return {}; }
    virtual TexturePtr getNormalMapTexture() { return nullptr; }
  };

  using TerrainFactory = util::Factory<TerrainBase, TextureManager&, const ShaderSearchPath&>;

  template <class T>
  TerrainFactory makeTerrainFactory()
  {
    return [] (TextureManager &tm, const ShaderSearchPath &shader_search_path)
    {
      return std::make_shared<T>(tm, shader_search_path);
    };
  }
}

#endif
