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

#extension GL_ARB_texture_query_lod : require

#define ENABLE_BASE_MAP @enable_base_map@
#define ENABLE_TERRAIN_NOISE 1
#define ENABLE_FAR_TEXTURE !@enable_base_map@
#define ENABLE_WATER 1
#define ENABLE_WATER_TYPE_MAP 1
#define ENABLE_FOREST 1
#define ENABLE_FAR_FOREST 0
#define DETAILED_FOREST false
#define ENABLE_TERRAIN_NORMAL_MAP 1
#define ENABLE_TYPE_MAP 1
#define LOW_DETAIL @low_detail:false@


float getDetailMapBlend(vec2 pos);
float genericNoise(vec2 coord);
vec3 calcLight(vec3 pos, vec3 normal);
vec4 getForestFarColor(vec2 pos);
vec4 getForestFarColorSimple(vec2 pos);
vec4 getForestColor(vec2 pos, int layer);
float getWaterDepth(vec2 pos);
void sampleWaterType(vec2 pos, out float shallow_sea_amount, out float river_amount);
vec4 applyWater(vec4 color,
  vec3 view_dir,
  float dist,
  float waterDepth,
  vec2 mapCoords,
  vec2 pos,
  float shallow_sea_amount,
  float river_amount,
  float bank_amount);

uniform float terrain_tile_size_m;

const float near_distance = 80000;

uniform bool draw_near_forest = false;
uniform bool enable_terrain_noise = false;

uniform sampler2D sampler_terrain_cdlod_normal_map;
uniform sampler2D sampler_terrain_cdlod_normal_map_base;
uniform vec2 height_map_base_size_m;
uniform vec2 height_map_base_origin;

uniform sampler2DArray sampler_terrain;
uniform sampler1D sampler_terrain_scale_map;
uniform sampler2D sampler_type_map;
uniform sampler2D sampler_type_map_base;
uniform sampler2D sampler_terrain_noise;
uniform sampler2D sampler_terrain_far;
uniform sampler2D sampler_shallow_water;
uniform sampler2DArray sampler_beach;

uniform ivec2 typeMapSize;
uniform vec2 map_size;
uniform vec3 cameraPosWorld;

varying vec2 pass_texcoord;

const ivec2 type_map_base_size_px = ivec2(4096);
const ivec2 type_map_base_meters_per_pixel = ivec2(400);

float sampleNoise(vec2 coord)
{
//   const float noise_strength = 2.0;
//   const float noise_strength = 4;

  float noise = texture2D(sampler_terrain_noise, coord).x;

//   noise = clamp((noise - 0.5) * 2, 0, 5);

  noise *= 2;

  return noise;
}


void sampleTypeMap(sampler2D sampler,
            ivec2 textureSize,
            vec2 coords,
            out float types[4],
            out float weights[4])
{
//   float x = coords.x - 0.5;
//   float y = coords.y - 0.5;

  float x = coords.x - 0.0;
  float y = coords.y - 0.0;


//   float x0 = floor(x + 0);
// //   float y0 = floor(y - 0.5);
  float x0 = floor(x);
  float y0 = floor(y);

  float x1 = x0 + 1.0;
  float y1 = y0 + 1.0;

  float x0_w = abs(x - x1);
  float x1_w = 1.0 - x0_w;

  float y0_w = abs(y - y1);
  float y1_w = 1.0 - y0_w;

  float w00 = x0_w * y0_w;
  float w01 = x0_w * y1_w;
  float w10 = x1_w * y0_w;
  float w11 = x1_w * y1_w;

  ivec2 p00 = ivec2(x0, y0);
  ivec2 p01 = ivec2(x0, y1);
  ivec2 p10 = ivec2(x1, y0);
  ivec2 p11 = ivec2(x1, y1);

//   // tiled
//   vec4 c00 = texelFetch(sampler, p00 % textureSize, 0);
//   vec4 c01 = texelFetch(sampler, p01 % textureSize, 0);
//   vec4 c10 = texelFetch(sampler, p10 % textureSize, 0);
//   vec4 c11 = texelFetch(sampler, p11 % textureSize, 0);

  // clamped
  vec4 c00 = texelFetch(sampler, p00, 0);
  vec4 c01 = texelFetch(sampler, p01, 0);
  vec4 c10 = texelFetch(sampler, p10, 0);
  vec4 c11 = texelFetch(sampler, p11, 0);


//ARTIFACTS!!!
//   vec2 p00 = vec2(x0, y0);
//   vec2 p01 = vec2(x0, y1);
//   vec2 p10 = vec2(x1, y0);
//   vec2 p11 = vec2(x1, y1);
//   vec4 c00 = texture(sampler, p00 / textureSize);
//   vec4 c01 = texture(sampler, p01 / textureSize);
//   vec4 c10 = texture(sampler, p10 / textureSize);
//   vec4 c11 = texture(sampler, p11 / textureSize);

  types[0] = c00.x;
  types[1] = c01.x;
  types[2] = c10.x;
  types[3] = c11.x;

  weights[0] = w00;
  weights[1] = w01;
  weights[2] = w10;
  weights[3] = w11;
}


vec4 sampleTerrain(float type_normalized)
{
  vec2 mapCoords = pass_texcoord;

  uint index = uint(type_normalized * 255) & 0x1Fu;
  float scale = texelFetch(sampler_terrain_scale_map, int(index), 0).x;

  return texture(sampler_terrain, vec3(mapCoords * scale, index));
}


vec4 sampleTerrainTextures(vec2 pos)
{
  float types[4];
  float weights[4];

  vec2 typeMapCoords = pos.xy / 200.0;

//   float lod = textureQueryLod(sampler_type_map, typeMapCoords).y;
//   float lod = mip_map_level(pos.xy / map_size);

  sampleTypeMap(sampler_type_map, typeMapSize, typeMapCoords, types, weights);

  vec4 c00 = sampleTerrain(types[0]);
  vec4 c01 = sampleTerrain(types[1]);
  vec4 c10 = sampleTerrain(types[2]);
  vec4 c11 = sampleTerrain(types[3]);

  return
    c00 * weights[0] +
    c01 * weights[1] +
    c10 * weights[2] +
    c11 * weights[3];
}


vec4 sampleBaseTerrain(float type_normalized)
{
  vec2 mapCoords = pass_texcoord;

  uint index = uint(type_normalized * 255);
//   float scale = texelFetch(sampler_terrain_scale_map, int(index), 0).x;
  float scale = 1.0;

  return texture(sampler_terrain, vec3(mapCoords * scale, index));
}


vec4 sampleBaseTerrainTextures(vec2 pos)
{
  float types[4];
  float weights[4];

  vec2 typeMapCoords = (pos.xy - height_map_base_origin) / type_map_base_meters_per_pixel;

  sampleTypeMap(sampler_type_map_base, type_map_base_size_px, typeMapCoords, types, weights);

  vec4 c00 = sampleBaseTerrain(types[0]);
  vec4 c01 = sampleBaseTerrain(types[1]);
  vec4 c10 = sampleBaseTerrain(types[2]);
  vec4 c11 = sampleBaseTerrain(types[3]);

  return
    c00 * weights[0] +
    c01 * weights[1] +
    c10 * weights[2] +
    c11 * weights[3];
}


vec4 applyForest(vec4 color, vec2 pos, vec3 view_dir, float dist)
{
  vec4 forest_far_simple = getForestFarColorSimple(pos.xy);

  vec4 far_simple_color = vec4(mix(color, forest_far_simple, forest_far_simple.a * 0.8).xyz, 1);

#if LOW_DETAIL
    return far_simple_color;
#else

  float lod = textureQueryLOD(sampler_terrain_far, pos.xy / map_size).x;

  float far_simple_blend = clamp(lod, 0, 1);
  far_simple_blend = mix(far_simple_blend, 1.0, smoothstep(30000.0, 45000.0, dist));

#if ENABLE_FAR_FOREST
  vec4 forest_far = getForestFarColor(pos.xy);
  vec4 far_color = color;
  const float far_max_alpha = 1.0;
//   forest_far.a = 1;
  far_color.xyz = mix(color.xyz, forest_far.xyz, forest_far.a * far_max_alpha);

  if (dist > 15000.0)
  {
    color = far_color;
  }
  else
#endif
  {
    vec4 forest_floor = getForestColor(pos.xy, 0);
    color.xyz = mix(color.xyz, forest_floor.xyz, forest_floor.a);

    if (dist > 5000.0 || !DETAILED_FOREST)
    {
      vec2 offset = 3 * (view_dir - vec3(0,0,1)).xy;

//     const float forest_layer_height = 3.0;    
  //       vec2 offset = abs(1 / view_dir.z) * view_dir.xy * forest_layer_height;
  //       offset = clamp(offset, vec2(-8), vec2(8));

      const int num_layers = 5;

      for (int i = 1; i < num_layers; i++)
      {
        vec2 coords = pos.xy + (float(i) * offset);

        vec4 layer = getForestColor(coords, i);

        color.xyz = mix(color.xyz, layer.xyz, layer.a);
      }
    }

#if ENABLE_FAR_FOREST
    float far_blend = smoothstep(10000.0, 14000.0, dist);
    color.xyz = mix(color.xyz, far_color.xyz, far_blend);
#endif
  }
  color = mix(color, far_simple_color, far_simple_blend);
  return color;

#endif
}


vec4 applyTerrainNoise(vec4 color, float dist)
{
//   const float noise_near_strength = 0.4;
//   const float noise_far_strength = 0.2;
//   const float noise_very_far_strength = 0.15;

  vec2 coords = pass_texcoord;

  const float noise_strength = 1.0;
  const float noise_near_strength = 0.4 * noise_strength;
  const float noise_far_strength = 0.4 * noise_strength;
  const float noise_very_far_strength = 0.4 * noise_strength;

//   color.xyz = vec3(0.5);

  float noise = sampleNoise(coords * 80);
  float noise_blend = noise_near_strength * (1 - smoothstep(0, 1000, dist));

//   float noise_far = sampleNoise(coords * 10);
  float noise_far = sampleNoise(coords * 20);
  float noise_far_blend = noise_far_strength * (1 - smoothstep(500, 2000, dist));
 
  float noise_very_far = sampleNoise(coords * 4);
  float noise_very_far_blend = noise_very_far_strength * (1 - smoothstep(100, 4000, dist));

  color = mix(color, color * noise_very_far, noise_very_far_blend);
  color = mix(color, color * noise_far, noise_far_blend);
  color = mix(color, color * noise, noise_blend);

  return color;
}


vec4 applyFarTexture(vec4 color, vec2 pos, float dist)
{
  vec2 mapCoordsFar = pos.xy / map_size;
  vec4 farColor = texture(sampler_terrain_far, mapCoordsFar);
  
//   float lod = mip_map_level(pos.xy / 200);
  float lod = textureQueryLOD(sampler_terrain_far, mapCoordsFar).x;
 
#if ENABLE_TYPE_MAP

#if !LOW_DETAIL
  {
  //   float farBlend = 1 - exp(-pow(3 * (dist / near_distance), 2));
  //   float farBlend = 1 - exp(-pow(3 * (dist / near_distance), 2));

  //   float farBlend = 1 - clamp(dot(view_dir, normal), 0, 1);
  //   farBlend = smoothstep(0.9, 1.0, farBlend);

  //   float farBlend = smoothstep(near_distance * 0.5, near_distance, dist);

    float farBlend = clamp(lod, 0, 1);
    farBlend = mix(farBlend, 1.0, smoothstep(30000.0, 45000.0, dist));

  //   farColor *= 0;

    color = mix(color, farColor, farBlend);

  //   if (gl_FragCoord.x > 900)
  //     color = farColor;
  }
#else
  color = farColor;
#endif

#else
  color = farColor;
#endif

  return color;
}


vec4 getTerrainColor(vec3 pos)
{
  float dist = distance(cameraPosWorld, pos);
  vec3 view_dir = normalize(cameraPosWorld - pos);

  float detail_blend = 1.0;

#if ENABLE_BASE_MAP
    detail_blend = getDetailMapBlend(pos.xy);
#endif

#if ENABLE_TERRAIN_NORMAL_MAP
  vec2 normal_map_coord = fract((pos.xy + vec2(0, 200)) / map_size);
  normal_map_coord.y = 1.0 - normal_map_coord.y;
  vec3 normal = texture2D(sampler_terrain_cdlod_normal_map, normal_map_coord).xyz;

#if ENABLE_BASE_MAP
  {
    vec2 normal_map_coord_base = fract((pos.xy - height_map_base_origin) / height_map_base_size_m);
    normal_map_coord_base.y = 1.0 - normal_map_coord_base.y;
    vec3 normal_base = texture2D(sampler_terrain_cdlod_normal_map_base, normal_map_coord_base).xyz;

    normal = mix(normal_base, normal, detail_blend);
  }
#endif

#else
//   vec3 normal = passNormal;
  vec3 normal = vec3(0,0,1);
#endif

  normal.y *= -1;

  vec3 light = calcLight(pos, normal); 

  float shallow_sea_amount = 0;
  float river_amount = 0;

#if ENABLE_WATER && ENABLE_WATER_TYPE_MAP
  sampleWaterType(pos.xy, shallow_sea_amount, river_amount);
#endif

//   shallow_sea_amount = 1;
//   river_amount = 0;

  vec4 color = vec4(0.7, 0.7 , 0.7 , 1.0);

//   float lod = mip_map_level(pos.xy / 200);

#if ENABLE_TYPE_MAP
#if !LOW_DETAIL
    color = sampleTerrainTextures(pos.xy);

#if ENABLE_BASE_MAP
  color = mix(sampleBaseTerrainTextures(pos.xy), color, detail_blend);
#endif
#endif
  color.w = 1;
#endif

//   color.xyz = vec3(1.0);

#if ENABLE_WATER
float waterDepth = 0;
float bank_amount = 0;

  waterDepth = getWaterDepth(pos.xy);

  vec3 shallowWaterColor = texture2D(sampler_shallow_water, pass_texcoord * 2).xyz;

  color.xyz = mix(color.xyz, shallowWaterColor, smoothstep(0.55, 0.9, waterDepth));

  vec4 bankColor = texture(sampler_beach, vec3(pass_texcoord * 5, 2));
  bank_amount = smoothstep(0.4, 0.45, waterDepth);
  bank_amount *= (1-shallow_sea_amount);
//   bank_amount *= 0.8;
  color.xyz = mix(color.xyz, bankColor.xyz, bank_amount);

//   color.xyz = mix(color.xyz, mix(bankColor.xyz, color.xyz, smoothstep(0.0, 0.05, terrain_height)), 0.9 * (1-shallow_sea_amount));
//   color.xyz = mix(color.xyz, color.xyz * 0.8, wetness);
#endif

#if ENABLE_TERRAIN_NOISE
#if !LOW_DETAIL
  color = applyTerrainNoise(color, dist);
#endif
#endif

#if ENABLE_FAR_TEXTURE
  color = applyFarTexture(color, pos.xy, dist);
#endif

#if ENABLE_FOREST
  color = applyForest(color, pos.xy, view_dir, dist);
#endif

  color.xyz *= light;

#if ENABLE_WATER
  color = applyWater(color, view_dir, dist, waterDepth, pass_texcoord, pos.xy, shallow_sea_amount, river_amount, bank_amount);
#endif

// DEBUG
//   if (fract(pos.xy / water_map_chunk_size_m).x < 0.002 || fract(pos.xy / water_map_chunk_size_m).y < 0.002)
//     color.xyz = vec3(1,0,0);

  return color;
}
