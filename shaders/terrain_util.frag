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

#define ENABLE_TERRAIN_NOISE 1
#define ENABLE_WATER_TYPE_MAP 1
#define ENABLE_FAR_FOREST 0
#define DETAILED_FOREST false
#define ENABLE_TERRAIN_NORMAL_MAP 1

// #define ENABLE_BASE_MAP @enable_base_map@
#define ENABLE_FAR_TEXTURE !@enable_base_map@
#define LOW_DETAIL @low_detail:0@
#define DETAILED_WATER @detailed_water:1@
#define ENABLE_WATER @enable_water:0@
#define ENABLE_FOREST @enable_forest:0@
#define ENABLE_TYPE_MAP @enable_type_map:0@
#define ENABLE_TERRAIN_DETAIL_NM !LOW_DETAIL && @enable_terrain_detail_nm:0@

#define ENABLE_TERRAIN0 @enable_terrain0:0@
#define ENABLE_TERRAIN1 @enable_terrain1:0@
#define ENABLE_TERRAIN2 @enable_terrain2:0@
#define ENABLE_TERRAIN3 @enable_terrain3:0@

#define ENABLE_TERRAIN_DETAIL_NM0 @enable_terrain_detail_nm0:0@
#define ENABLE_TERRAIN_DETAIL_NM1 @enable_terrain_detail_nm1:0@
#define ENABLE_TERRAIN_DETAIL_NM2 @enable_terrain_detail_nm2:0@
#define ENABLE_TERRAIN_DETAIL_NM3 @enable_terrain_detail_nm3:0@


vec3 calcLightWithDetail(vec3 normal, vec3 normal_detail, float direct_scale, float ambient_scale);
vec3 textureColorCorrection(vec3 color);
float getDetailMapBlend(vec2 pos);
float genericNoise(vec2 coord);
vec3 calcLight(vec3 pos, vec3 normal, float direct_scale, float ambient_scale);
vec4 getForestFarColor(vec2 pos);
vec4 getForestFarColorSimple(vec2 pos);
vec4 getForestColor(vec2 pos, int layer);
vec4 getForestColorFloat(vec2 pos, float layer);
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

uniform sampler2D sampler_terrain_cdlod_normal_map;
uniform sampler2D sampler_terrain_cdlod_normal_map_base;
uniform vec2 height_map_base_size_m;
uniform vec2 height_map_base_origin;

uniform sampler1D sampler_terrain_scale_map;
uniform sampler2D sampler_type_map;
uniform sampler2D sampler_type_map_normals;
uniform sampler2D sampler_type_map_base;
uniform sampler2D sampler_terrain_noise;
uniform sampler2D sampler_terrain_far;
uniform sampler2D sampler_shallow_water;
uniform sampler2DArray sampler_beach;

uniform sampler2D sampler_terrain_cdlod_height_map;
uniform float terrain_resolution_m;
uniform vec2 height_map_size_m;

uniform ivec2 typeMapSize;
uniform vec2 map_size;
uniform vec3 cameraPosWorld;

uniform sampler2DArray sampler_terrain;
uniform sampler2DArray sampler_terrain1;
uniform sampler2DArray sampler_terrain2;
uniform sampler2DArray sampler_terrain3;

uniform sampler2DArray sampler_terrain_detail_nm0;
uniform sampler2DArray sampler_terrain_detail_nm1;
uniform sampler2DArray sampler_terrain_detail_nm2;
uniform sampler2DArray sampler_terrain_detail_nm3;

varying vec2 pass_texcoord;
varying vec2 pass_type_map_coord;

const ivec2 type_map_base_size_px = ivec2(4096);
const ivec2 type_map_base_meters_per_pixel = ivec2(400);


//FIXME duplication
float getTerrainHeight(vec2 world_coord)
{
  vec2 height_map_coord = (world_coord + vec2(0, terrain_resolution_m)) / height_map_size_m;
  height_map_coord.y = 1.0 - height_map_coord.y;

  return textureLod(sampler_terrain_cdlod_height_map, height_map_coord, 0).x;
}


float sampleNoise(vec2 coord)
{
//   const float noise_strength = 2.0;
//   const float noise_strength = 4;

  float noise = texture2D(sampler_terrain_noise, coord).x;

  noise *= 2;

  return noise;
}


void sampleTypeMap(sampler2D sampler,
            ivec2 textureSize,
            vec2 coords,
            out vec3 types[4],
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

  types[0] = c00.xyz;
  types[1] = c01.xyz;
  types[2] = c10.xyz;
  types[3] = c11.xyz;

  weights[0] = w00;
  weights[1] = w01;
  weights[2] = w10;
  weights[3] = w11;
}


vec4 sampleTerrain(vec3 type)
{
  float sampler_nr = type.x * 255;
  uint index = uint(type.y * 255);
  float scale = (type.z * 255) - 128;
  if (scale < 0)
    scale = 1 / -scale;
  vec4 color = vec4(0);

#if ENABLE_TERRAIN0
  color += texture(sampler_terrain, vec3(pass_texcoord * scale, index)) * float(sampler_nr == 0);
#endif
#if ENABLE_TERRAIN1
  color += texture(sampler_terrain1, vec3(pass_texcoord * scale, index)) * float(sampler_nr == 1);
#endif
#if ENABLE_TERRAIN2
  color += texture(sampler_terrain2, vec3(pass_texcoord * scale, index)) * float(sampler_nr == 2);
#endif
#if ENABLE_TERRAIN3
  color += texture(sampler_terrain3, vec3(pass_texcoord * scale, index)) * float(sampler_nr == 3);
#endif

  color.a = 1;

  return color;
}


#if ENABLE_TERRAIN_DETAIL_NM
vec3 sampleTerrainDetailNormal(vec3 type)
{
  float sampler_nr = type.x * 255;
  uint index = uint(type.y * 255);
  float scale = (type.z * 255) - 128;
  if (scale < 0)
    scale = 1 / -scale;
  vec4 color = vec4(0);

#if ENABLE_TERRAIN_DETAIL_NM0
  color += texture(sampler_terrain_detail_nm0, vec3(pass_texcoord * scale, index)) * float(sampler_nr == 0);
#endif
#if ENABLE_TERRAIN_DETAIL_NM1
  color += texture(sampler_terrain_detail_nm1, vec3(pass_texcoord * scale, index)) * float(sampler_nr == 1);
#endif
#if ENABLE_TERRAIN_DETAIL_NM2
  color += texture(sampler_terrain_detail_nm2, vec3(pass_texcoord * scale, index)) * float(sampler_nr == 2);
#endif
#if ENABLE_TERRAIN_DETAIL_NM3
  color += texture(sampler_terrain_detail_nm3, vec3(pass_texcoord * scale, index)) * float(sampler_nr == 3);
#endif

  vec3 normal = color.xyz * 2 - 1;
  normal.y *= -1;

  if (sampler_nr == 255)
    normal = vec3(0,0,1);

  return normal;
}
#endif


vec4 sampleTerrainTextures(vec2 pos)
{
  vec3 types[4];
  float weights[4];

//   float lod = textureQueryLod(sampler_type_map, typeMapCoords).y;
//   float lod = mip_map_level(pos.xy / map_size);

  sampleTypeMap(sampler_type_map, typeMapSize, pass_type_map_coord, types, weights);

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


#if ENABLE_TERRAIN_DETAIL_NM
vec3 getTerrainDetailNormal(vec2 pos)
{
  vec3 types[4];
  float weights[4];

  sampleTypeMap(sampler_type_map_normals, typeMapSize, pass_type_map_coord, types, weights);

  vec3 c00 = sampleTerrainDetailNormal(types[0]);
  vec3 c01 = sampleTerrainDetailNormal(types[1]);
  vec3 c10 = sampleTerrainDetailNormal(types[2]);
  vec3 c11 = sampleTerrainDetailNormal(types[3]);

  return
    c00 * weights[0] +
    c01 * weights[1] +
    c10 * weights[2] +
    c11 * weights[3];
}
#endif


#if ENABLE_BASE_MAP
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
#endif


//FIXME also in atmosphere.frag
vec3 intersect__(vec3 lineP,
               vec3 lineN,
               vec3 planeN,
               float  planeD,
               float max_dist,
               out float dist)
{
    dist = (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
    return lineP + lineN * min(dist, max_dist);
}

vec4 applyForest(vec4 color, vec3 pos, vec3 view_dir, float dist, vec3 normal)
{
  vec4 forest_far_simple = getForestFarColorSimple(pos.xy);

  vec4 far_simple_color = vec4(mix(color, forest_far_simple, forest_far_simple.a * 0.8).xyz, 1);

#if LOW_DETAIL
    return far_simple_color;
#else

#if ENABLE_FAR_TEXTURE
  float lod = textureQueryLOD(sampler_terrain_far, pos.xy / map_size).x;
#else
  float lod = 0;
#endif

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
//     color.xyz = mix(color.xyz, forest_floor.xyz, forest_floor.a);

    if (dist > 5000.0 || !DETAILED_FOREST)
    {
      const int num_steps = 20;
      const float step_size = 4.0;
    
      const int num_layers = 5;

      vec2 offset = 3 * (view_dir - vec3(0,0,1)).xy;

      const float forest_layer_height = 3.0;
      const float forest_height = num_layers * forest_layer_height;
//       vec2 offset = abs(1 / view_dir.z) * view_dir.xy * forest_layer_height;
  //       offset = clamp(offset, vec2(-8), vec2(8));

  
      vec3 forest_top = pos;
      forest_top.z = getTerrainHeight(pos.xy);

//       const float max_dist = 50;
//       float dist;
//       intersect__(cameraPosWorld, view_dir, normal, height, max_dist, dist);

      vec3 combined_transparency = vec3(1);
      vec3 combined_color = vec3(0);

      for (int i = 0; i < num_steps; i++)
      {
        vec3 step_pos = forest_top + (normalize(view_dir) * i * step_size);
        float forest_bottom = getTerrainHeight(step_pos.xy) - forest_height;
//         float forest_bottom = forest_top.z - forest_height;
        
        float height = step_pos.z - forest_bottom;
        
        if (height < 0)
          break;
        if (step_pos.z > forest_top.z)
          break;


        vec3 c = vec3(1,0,0);
        c = mix(c, vec3(0,1,0), step(3, height));
        c = mix(c, vec3(0,0,1), step(6, height));
        c = mix(c, vec3(1,1,0), step(9, height));
        c = mix(c, vec3(0,1,1), step(12, height));
      
        float layer_num = (height / forest_height) * num_layers;
//         layer_num = 1;

//         vec2 coords = pos.xy + (float(i) * offset);
        
        
//         float height = pos.z + i * forest_layer_height;
        
//         
        
//         float dist;
//         vec3 coords = intersect__(cameraPosWorld, view_dir, normal, height, max_dist, dist);
        

        vec4 layer = getForestColorFloat(step_pos.xy, layer_num);

//         layer.xyz = c;

//         combined_color = mix(layer.xyz, combined_color, combined_alpha);
        combined_color = mix(layer.xyz, combined_color, vec3(1) - combined_transparency);
        
        combined_transparency *= 1-layer.a;
        
//         color.xyz = mix(color.xyz, vec3(1), layer.a);

//         combined_alpha = mix(combined_alpha, vec3(1), layer.xyz * layer.a);
//         combined_alpha = clamp(combined_alpha + layer.xyz * layer.a, vec3(0), vec3(1));
//         combined_alpha = clamp(combined_alpha + vec3(layer.a), vec3(0), vec3(1));
        
        
//         color.xyz = mix(color.xyz, layer.xyz, layer.a);
        
      }
      
//       color.xyz = mix(color.xyz, combined_color, combined_alpha);
//       color.xyz = mix(color.xyz, vec3(1), vec3(1) - combined_transparency);
      color.xyz = mix(color.xyz, combined_color, vec3(1) - combined_transparency);
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
  vec2 coords = pass_texcoord;

  color = mix(color, color * sampleNoise(coords * 32), 0.5 * (1 - smoothstep(500, 1000, dist)));
  color = mix(color, color * sampleNoise(coords * 128), 1.0 * (1 - smoothstep(100, 200, dist)));

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
  vec3 view_dir = -normalize(cameraPosWorld - pos);

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

  float shallow_sea_amount = 0;
  float river_amount = 0;

#if ENABLE_WATER && ENABLE_WATER_TYPE_MAP
  sampleWaterType(pos.xy, shallow_sea_amount, river_amount);
#endif

//   shallow_sea_amount = 1;
//   river_amount = 0;

  vec4 color = vec4(0.7, 0.7 , 0.7 , 1.0);

//   float lod = mip_map_level(pos.xy / 200);

vec3 normal_detail = vec3(0,0,1);

#if ENABLE_TYPE_MAP
  #if !LOW_DETAIL
    color = sampleTerrainTextures(pos.xy);
    #if ENABLE_BASE_MAP
      color = mix(sampleBaseTerrainTextures(pos.xy), color, detail_blend);
    #endif
    #if ENABLE_TERRAIN_DETAIL_NM
      normal_detail = getTerrainDetailNormal(pos.xy);
    #endif
  #endif
  color.w = 1;
#endif

#if ENABLE_TERRAIN_DETAIL_NM
  vec3 light = calcLightWithDetail(normal, normal_detail, 1, 1);
#else
  vec3 light = calcLight(vec3(0), normal, 1, 1);
#endif

#if ENABLE_WATER
float waterDepth = 0;
float bank_amount = 0;

  waterDepth = getWaterDepth(pos.xy);

  vec3 shallowWaterColor = texture2D(sampler_shallow_water, pass_texcoord * 2).xyz;

  color.xyz = mix(color.xyz, shallowWaterColor, smoothstep(0.55, 0.9, waterDepth));

#if DETAILED_WATER
  vec4 bankColor = texture(sampler_beach, vec3(pass_texcoord * 5, 2));
  bank_amount = smoothstep(0.4, 0.45, waterDepth);
  bank_amount *= (1-shallow_sea_amount);
//   bank_amount *= 0.8;
  color.xyz = mix(color.xyz, bankColor.xyz, bank_amount);
#endif

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
//   color = applyForest(color, pos.xyz, view_dir, dist, normal);
#endif

  color.xyz = textureColorCorrection(color.xyz);

  color.xyz *= light;

#if ENABLE_WATER
  color = applyWater(color, view_dir, dist, waterDepth, pass_texcoord, pos.xy, shallow_sea_amount, river_amount, bank_amount);
#endif

// DEBUG
//   if (fract(pos.xy / water_map_chunk_size_m).x < 0.002 || fract(pos.xy / water_map_chunk_size_m).y < 0.002)
//     color.xyz = vec3(1,0,0);

  return color;
}
