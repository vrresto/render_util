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

struct TerrainTextureMap
{
  sampler2D sampler;
  int resolution_m;
  ivec2 size_px;
  vec2 size_m;
};


struct WaterMap
{
  sampler2D sampler_table;
  sampler2DArray sampler;

  ivec2 table_size_px;
  vec2 table_size_m;
  vec2 table_shift_m;
  int chunk_size_m;
  vec2 scale;
  vec2 shift;
};

struct TerrainLayer
{
  vec2 size_m;
  vec2 origin_m;
  TerrainTextureMap height_map;
  TerrainTextureMap normal_map;

#if @enable_type_map@
  TerrainTextureMap type_map;
#endif

#if @enable_forest@
  TerrainTextureMap forest_map;
#endif

  WaterMap water_map;
};

struct Terrain
{
  int mesh_resolution_m;
  int tile_size_m;
  float max_texture_scale;
  TerrainLayer detail_layer;
#if @enable_base_map@
  TerrainLayer base_layer;
#endif
};


#if @enable_type_map@
  ivec2 getTypeMapSizePx();
  sampler2D getTypeMapSampler();
  #if @enable_base_map@
    sampler2D getBaseTypeMapSampler();
    ivec2 getBaseTypeMapSizePx();
  #endif
#endif

// vec2 getWaterMapCoords(in WaterMap map);
// vec2 getWaterMapTableCoords(in WaterMap map);
float sampleWaterMap(vec2 pos, in TerrainLayer layer);
