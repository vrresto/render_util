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

#version 330

#include terrain_params.h.glsl
#include terrain_geometry_util.h.glsl

uniform Terrain terrain;

#if @enable_base_map@
uniform float terrain_base_map_height = 0.0;
#endif


vec2 getHeightMapTextureCoords(in TerrainLayer layer, vec2 pos_m)
{
  vec2 coords =
    (pos_m - layer.origin_m.xy + vec2(0, layer.height_map.resolution_m)) / layer.height_map.size_m;
  coords.y = 1.0 - coords.y;
  return coords;
}


vec2 getNormalMapCoords(in TerrainLayer layer, vec2 pos_m)
{
  vec2 coords =
    (pos_m - layer.origin_m.xy + vec2(0, layer.normal_map.resolution_m))
      / layer.normal_map.size_m;
  coords.y = 1.0 - coords.y;
  return coords;
}


vec3 sampleTerrainNormalMap(in TerrainLayer layer, vec2 pos_m)
{
  vec2 coords = getNormalMapCoords(layer, pos_m);
  vec3 normal = texture2D(layer.normal_map.sampler, coords).xyz;
  normal.y *= -1;
  return normal;
}


float getTerrainHeight(vec2 pos_m)
{
  vec2 height_map_texture_coords = getHeightMapTextureCoords(terrain.detail_layer, pos_m);
  float detail = texture2D(terrain.detail_layer.height_map.sampler, height_map_texture_coords).x;

#if @enable_base_map@
  vec2 base_height_map_texture_coords = getHeightMapTextureCoords(terrain.base_layer, pos_m);
  float base = texture2D(terrain.base_layer.height_map.sampler, base_height_map_texture_coords).x;
  base += terrain_base_map_height;

  float detail_blend = getDetailMapBlend(pos_m);

  return mix(base, detail, detail_blend);
#else
  return detail;
#endif
}


float getDetailMapBlend(vec2 pos)
{
  vec2 map_size = terrain.detail_layer.size_m;

  float blend_dist = 10000.0;

//   float threshold_noise = clamp(genericNoise(pos * 0.0008), -1, 1);
//   float threshold_noise_coarse = clamp(genericNoise(pos * 0.00005), -1, 1);

//   float threshold = 0.5;

//   threshold -= 0.5 * threshold_noise_coarse;
//   threshold += 0.1 * threshold_noise;

//   threshold = clamp(threshold, 0.1, 0.9);

  float detail_blend_x =
    smoothstep(0.0, blend_dist, pos.x) -
    smoothstep(map_size.x - blend_dist, map_size.x, pos.x);

  float detail_blend_y =
    smoothstep(0.0, blend_dist, pos.y) -
    smoothstep(map_size.y - blend_dist, map_size.y, pos.y);

  float detail_blend = detail_blend_x * detail_blend_y;


//   detail_blend = smoothstep(threshold, threshold + 0.4, detail_blend);


//   detail_blend *= smoothstep(0.5, 0.6, detail_blend);

  return detail_blend;
}
