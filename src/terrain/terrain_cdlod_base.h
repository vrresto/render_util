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

/**
 * This terrain implementation makes use of the technique described in the paper
 * "Continuous Distance-Dependent Level of Detail for Rendering Heightmaps (CDLOD)"
 * by Filip Strugar <http://www.vertexasylum.com/downloads/cdlod/cdlod_latest.pdf>.
 */

#ifndef RENDER_UTIL_TERRAIN_TERRAIN_CDLOD_BASE_H
#define RENDER_UTIL_TERRAIN_TERRAIN_CDLOD_BASE_H


#include <render_util/terrain_base.h>

namespace render_util
{


class TerrainCDLODBase : public TerrainBase
{
public:
  static constexpr int METERS_PER_GRID = render_util::TerrainBase::GRID_RESOLUTION_M; //FIXME remove
  static constexpr float MIN_LOD_DIST = 40000;
  static constexpr unsigned int MESH_GRID_SIZE = 64;
  static constexpr auto LOD_LEVELS = 8;
  static constexpr auto HEIGHT_MAP_METERS_PER_GRID = 200;
  static constexpr auto LEAF_NODE_SIZE = MESH_GRID_SIZE * METERS_PER_GRID;
  static constexpr auto MAX_LOD = LOD_LEVELS;


  class MaterialMap
  {
    struct Map;

    std::unique_ptr<Map> createMap(const TerrainBase::Loader::Layer&);

    std::unique_ptr<Map> m_map;
    std::unique_ptr<Map> m_base_map;

  public:
    MaterialMap(const TerrainBase::BuildParameters &params);
    ~MaterialMap();
    unsigned int getMaterialID(const Rect &area) const;
  };


  static constexpr size_t getNumLeafNodes()
  {
    return pow(4, (size_t)LOD_LEVELS);
  }


  static constexpr float getLodLevelDist(int lod_level)
  {
    return MIN_LOD_DIST * pow(2, lod_level);
  }


  static float getMaxHeight(const TerrainBase::BuildParameters&, const glm::vec2 &pos, float size)
  {
    return 4000.0;
  }


  static constexpr double getNodeSize(int lod_level)
  {
    return pow(2, lod_level) * LEAF_NODE_SIZE;
  }


  static constexpr double getNodeScale(int lod_level)
  {
    return pow(2, lod_level);
  }

  static TexturePtr createNormalMapTexture(render_util::ElevationMap::ConstPtr, int meters_per_grid);
  static TexturePtr createHeightMapTexture(render_util::ElevationMap::ConstPtr);
};


static_assert(TerrainCDLODBase::getNodeSize(0) == TerrainCDLODBase::LEAF_NODE_SIZE);
static_assert(TerrainCDLODBase::getNodeScale(0) == 1);


} // namespace render_util

#endif
