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

#version 130

#define ENABLE_BASE_MAP @enable_base_map@

#include terrain_params.h.glsl

float genericNoise(vec2 coord);
float getDetailMapBlend(vec2 pos);

uniform ivec2 typeMapSize;

uniform sampler2D sampler_forest_map;
#if ENABLE_BASE_MAP
  uniform sampler2D sampler_forest_map_base;
#endif
uniform sampler2D sampler_forest_far;
uniform sampler2DArray sampler_forest_layers;

uniform Terrain terrain;


float sampleForestAlpha(vec2 typeMapCoords, vec2 pos)
{
  float forest_alpha = texture2D(sampler_forest_map, (typeMapCoords + vec2(0.5)) / typeMapSize).x;

  #if ENABLE_BASE_MAP
  {
    vec2 coords = (pos - terrain.base_layer.origin_m) / terrain.base_layer.size_m;
    float forest_alpha_base =
      texture2D(terrain.base_layer.forest_map.sampler, coords).x;
    forest_alpha = mix(forest_alpha_base, forest_alpha, getDetailMapBlend(pos));
  }
  #endif

  return forest_alpha;
}

float getForestAlpha(vec2 typeMapCoords, vec2 pos)
{

  float forest_alpha = sampleForestAlpha(typeMapCoords, pos);

  float forest_noise = genericNoise(typeMapCoords * 1.0);
//   forest_noise += 0.5 * genericNoise(typeMapCoords * 2);
  float forest_noise_detail = genericNoise(typeMapCoords * 5.0);
//   forest_noise_detail = 0;
  forest_noise_detail += 0.5 * genericNoise(typeMapCoords * 10.0);

  float forest_alpha_threshold = 0.2;
  forest_alpha_threshold += forest_noise * 0.6;
  forest_alpha_threshold += forest_noise_detail * 0.3;

  forest_alpha = smoothstep(forest_alpha_threshold, forest_alpha_threshold + 0.05, forest_alpha);

  return forest_alpha;
}


vec4 getForestFarColor(vec2 pos)
{
  vec2 typeMapCoords = pos.xy / 200.0;
  vec2 forestCoords = pos.xy / 200;

  float alpha = getForestAlpha(typeMapCoords, pos);

  vec4 forest = texture(sampler_forest_far, forestCoords);

  forest.a *= alpha;

  return forest;
}


vec4 getForestFarColorSimple(vec2 pos)
{
  vec2 typeMapCoords = pos.xy / 200.0;
  vec2 forestCoords = pos.xy / 200;

  float alpha = sampleForestAlpha(typeMapCoords, pos);

  vec4 forest = texture(sampler_forest_far, forestCoords);

  forest.a *= alpha;

  return forest;
}


vec4 getForestColor(vec2 pos, int layer)
{
  vec2 typeMapCoords = pos.xy / 200.0;

  vec2 forestCoords = pos.xy / 200;

  float forest_alpha = getForestAlpha(typeMapCoords, pos);

//   float forest_alpha = texture2D(sampler_forest_map, (typeMapCoords + vec2(0.5)) / typeMapSize).x;
// 
//   float forest_noise = genericNoise(forestCoords * 1.0);
// //   forest_noise += 0.5 * genericNoise(forestCoords * 2);
//   float forest_noise_detail = genericNoise(forestCoords * 5.0);
// //   forest_noise_detail = 0;
//   forest_noise_detail += 0.5 * genericNoise(forestCoords * 10.0);
// 
//   float forest_alpha_threshold = 0.2;
//   forest_alpha_threshold += forest_noise * 0.6;
//   forest_alpha_threshold += forest_noise_detail * 0.3;
// 
//   forest_alpha = smoothstep(forest_alpha_threshold, forest_alpha_threshold + 0.05, forest_alpha);

  vec4 color = texture(sampler_forest_layers, vec3(forestCoords, layer));
  color.a *= forest_alpha;

  return color;
}
