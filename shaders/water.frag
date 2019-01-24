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
 *    Used techiques
 *
 *    Reoriented Normal Mapping:
 *    http://blog.selfshadow.com/publications/blending-in-detail/
 *
 *    Schlick's approximation:
 *    http://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
 */
 
 
#version 130

#define LOW_DETAIL !@detailed_water:1@
#define ENABLE_WAVES !LOW_DETAIL
#define ENABLE_WAVE_INTERPOLATION 1
// #define ENABLE_WAVE_FOAM 1
#define ENABLE_WATER_MAP 1
// #define ENABLE_SHORE_WAVES 1

#define ENABLE_BASE_MAP @enable_base_map@
#define ENABLE_BASE_WATER_MAP @enable_base_water_map@

// const float water_map_chunk_size_m = 1600;
const float water_map_chunk_size_m = 1600 * 4;
const vec2 water_map_table_shift = vec2(0, 200);
const float SPEC_HARDNESS = 256.0;
// const float sea_roughness = 0.4;
const float sea_roughness = 1.0;
const float PI = 3.14159265359;
const float ior_water = 1.333;
const float schlick_r0_water = pow(ior_water - 1, 2) / pow(ior_water + 1, 2);


float getDetailMapBlend(vec2 pos);
vec3 calcWaterLight(vec3 normal);
vec3 calcLight(vec3 pos, vec3 normal);
vec2 rotate(vec2 v, float a);
float perlin(vec2 p, float dim);
float genericNoise(vec2 coord);


uniform sampler2DArray sampler_beach;
uniform sampler2DArray sampler_foam_mask;
uniform sampler2DArray sampler_water_normal_map;
uniform sampler2D sampler_shallow_water;
uniform sampler2D sampler_deep_water;
uniform sampler2D sampler_water_map_simple;
uniform sampler2D sampler_shore_wave;

uniform sampler2D sampler_water_type_map;
uniform sampler2D sampler_water_map_table;
uniform sampler2DArray sampler_water_map;

#if ENABLE_BASE_MAP
  uniform vec2 height_map_base_size_m;
  uniform vec2 height_map_base_origin;
  #if ENABLE_BASE_WATER_MAP
    uniform sampler2D sampler_water_map_base;
  #endif
#endif

uniform vec3 sunDir;
uniform vec3 water_color;
uniform vec2 map_size;

uniform vec3 cameraPosWorld;

struct WaterAnimationParameters
{
  float frame_delta;
  int pos;
};

uniform WaterAnimationParameters water_animation_params[2];

uniform int water_animation_num_frames = 0;
uniform vec2 water_map_shift = vec2(0);
uniform vec2 water_map_scale = vec2(1);
uniform ivec2 water_map_table_size;
uniform vec4 shore_wave_scroll;
uniform ivec2 typeMapSize;

varying vec2 pass_texcoord;
varying vec2 pass_type_map_coord;


float calcSpecular(vec3 view_dir, vec3 normal, float hardness)
{
    vec3 R = reflect(view_dir, normal);
    vec3 lVec = -sunDir;
    
    return pow(max(dot(R, lVec), 0.0), hardness);
}


float fresnelSchlick(vec3 incoming, vec3 normal, float r0)
{
  float cos_angle = dot(normal, incoming);
  float r = r0 + (1 - r0) * pow(1 - cos_angle, 5);
  return clamp(r, 0, 1);
}


vec3 interpolateWaterAnimationFrames(vec3 texA, vec3 texB, vec3 texC, int layer)
{
  vec3 phase1A = texA;
  vec3 phase1B = texB;
  vec3 phase2A = texA;
  vec3 phase2B = texB;

  float waterAnimationFrameDelta = water_animation_params[layer].frame_delta;

  float waterAnimationFrameDelta2 = waterAnimationFrameDelta + 0.5;
  if (waterAnimationFrameDelta2 > 1.0)
  {
    waterAnimationFrameDelta2 -= 1.0;
    phase2A = texB;
    phase2B = texC;
  }

  vec3 phase1 = mix(phase1A, phase1B, waterAnimationFrameDelta);
  vec3 phase2 = mix(phase2A, phase2B, waterAnimationFrameDelta2);

  return mix(phase1, phase2, 0.5);
}


int getWaterAnimationPos(int offset, int layer)
{
  return (water_animation_params[layer].pos + offset) % water_animation_num_frames;
}


// float getFoamAmount(vec2 coord, int animation_offset)
// {
//   vec3 texA = texture(sampler_foam_mask, vec3(coord * 2, getWaterAnimationPos(0 + animation_offset))).xyz;
//   vec3 texB = texture(sampler_foam_mask, vec3(coord * 2, getWaterAnimationPos(1 + animation_offset))).xyz;
//   vec3 texC = texture(sampler_foam_mask, vec3(coord * 2, getWaterAnimationPos(2 + animation_offset))).xyz;
//
//   return interpolateWaterAnimationFrames(texA, texB, texC).x;
// }


float getNoise(vec2 coord)
{
  const float scale = 8;

//   wave_foam_amount = mix(wave_foam_amount, getFoamAmount(mapCoords + 0.5), noise2D(mapCoords));
  float noise = 1 *
    perlin(vec2((shore_wave_scroll.x * 10 + coord * scale)), 1)
//     *
//     +
//     noise2D(vec2(shore_wave_scroll.x * 1.5) + coord * 0.3)
    ;
    
  noise += perlin(vec2(shore_wave_scroll.x + coord * scale) + vec2(0.8), 1) ;
  noise += perlin(vec2(shore_wave_scroll.x + coord * scale) + vec2(1.5), 1) ;
  noise *= 2.7;  
  noise = clamp(noise, 0, 1);

  return noise;
}


float getFoamAmountWithNoise(vec2 coord)
{
#if ENABLE_WAVE_FOAM
  const float scale = 8;

//   wave_foam_amount = mix(wave_foam_amount, getFoamAmount(mapCoords + 0.5), noise2D(mapCoords));
  float noise = 1 *
    perlin(vec2((shore_wave_scroll.x * 10 + coord * scale)), 1)
//     *
//     +
//     noise2D(vec2(shore_wave_scroll.x * 1.5) + coord * 0.3)
    ;

  noise += perlin(vec2(shore_wave_scroll.x + coord * scale) + vec2(0.8), 1) ;
  noise += perlin(vec2(shore_wave_scroll.x + coord * scale) + vec2(1.5), 1) ;
  noise *= 2.7;  
  noise = clamp(noise, 0, 1);
//   noise = 1-noise;

  noise = getNoise(coord);
  
//   noise += getNoise(coord + 0.5);
  
  float amount1 = getFoamAmount(coord, 0);
  float amount2 = getFoamAmount(rotate(coord * 1.5, 0.2), 30);
  
//   amount2 = 0.0;
  
//   float amount = mix(amount1, amount2, noise);

  float amount = amount1 * noise;
//   float amount = amount1;
  
//   return noise;
  return amount;
#else
  return 0.0;
#endif
}


float getShoreWaveNoise(float pos)
{
  return sin(pos * 5);
}


float sampleShoreWave(vec2 pos, float waterDepth, float offset)
{
  const float shore_wave_length = 600;
  const float rotation = 0.25 * PI;

  pos = (rotate(pos, rotation));

  float sampling_pos = (pos.y / shore_wave_length);
  float noise_sampling_pos = (pos.x / shore_wave_length) + sampling_pos;
  
  float noise = getShoreWaveNoise(noise_sampling_pos);

  sampling_pos += offset;
  
  sampling_pos += noise * 0.03;  


//   float dist_from_coast = (waterDepth - 0.5) * 2;
  float dist_from_coast = clamp(waterDepth - 0.35, 0, 1) * (1/0.65);

//   sampling_pos.y -= pow(1 - clamp(clamp(waterDepth - 0.5, 0, 1) * 2, 0, 1), 4);

  sampling_pos -= 0.5 * pow(1 - dist_from_coast, 2);

//   sampling_pos -= 2 * exp(-0.6 * dist_from_coast);

  
//   sampling_pos -= 3.5 * (1 - smoothstep(dist_from_coast, 0.0, 1.0));

  
  float shore_wave_strength = texture(sampler_shore_wave, vec2(sampling_pos, 0)).x;
//   float shore_wave_strength = pow(1 - fract(sampling_pos), 4);
  
  return shore_wave_strength;
  
//   float shore_wave_strength2 = texture(sampler_shore_wave, vec2(0, shore_wave_scroll)).x;
//   float shore_wave_strength2_w = pow(1 - (clamp(waterDepth - 0.4, 0, 1) * 2), 2);
//   shore_wave_strength2_w *= 2;
//   return mix(shore_wave_strength, shore_wave_strength2, shore_wave_strength2_w);
}


// see http://blog.selfshadow.com/publications/blending-in-detail/
vec3 blend_rnm(vec3 n1, vec3 n2)
{
  n1 = (n1 + vec3(1)) / 2;
  n2 = (n2 + vec3(1)) / 2;

  vec3 t = n1.xyz*vec3( 2,  2, 2) + vec3(-1, -1,  0);
  vec3 u = n2.xyz*vec3(-2, -2, 2) + vec3( 1,  1, -1);
  vec3 r = t*dot(t, u) - u*t.z;

  return normalize(r);
}


vec3 blendNormal(vec3 n1, vec3 n2, float dist, float vis, float strength)
{
  float detailFactor = exp(-pow(3 * (dist/vis), 2));
  
//   float detailFactor = 1 - smoothstep(vis / 2, vis, dist);

  strength *= sea_roughness;

  n2.xy *= detailFactor * strength;

  return blend_rnm(n1, normalize(n2));
}


vec3 sampleWaterNormalMap(float scale, vec2 coord, int layer)
{
#if ENABLE_WAVE_INTERPOLATION
  vec3 texA = texture(sampler_water_normal_map, vec3(coord * scale, getWaterAnimationPos(0, layer))).xyz;
  vec3 texB = texture(sampler_water_normal_map, vec3(coord * scale, getWaterAnimationPos(1, layer))).xyz;
  vec3 texC = texture(sampler_water_normal_map, vec3(coord * scale, getWaterAnimationPos(2, layer))).xyz;

  vec3 normal = interpolateWaterAnimationFrames(texA, texB, texC, layer) * 2 - 1;
  normal.y *= -1;

  return normal;
#else
  return texture(sampler_water_normal_map, vec3(coord * scale, getWaterAnimationPos(0))).xyz * 2 - 1;
#endif
}


vec3 getWaterNormal(float dist, vec2 coord)
{
  vec3 normal = vec3(0,0,1);
#if ENABLE_WAVES
  const float bigWaveStrength = 0.01;
  const float mediumWaveStrength = 0.1;
  const float smallWaveStrength = 0.1;

  normal = blendNormal(normal, sampleWaterNormalMap(4, coord, 0), dist, 30000, mediumWaveStrength);
  normal = blendNormal(normal, sampleWaterNormalMap(20, coord, 1), dist, 15000, smallWaveStrength);
//     normal = blendNormal(normal, sampleWaterNormalMap(140, coord), dist, 500, 0.1);

  normal = normalize(normal);
#endif

  return normal;
}


vec3 getWaterColorSimple(vec3 viewDir, float dist)
{
  float ambientLight = 0.3 * smoothstep(-0.5, 0.4, sunDir.z);
  float directLight = 1.2;

  vec3 directLightColor = vec3(1.0, 1.0, 0.8);
  vec3 directLightColorLow = vec3(1.0, 0.6, 0.2);
  directLightColor = mix(directLightColorLow, directLightColor, smoothstep(-0.1, 0.4, sunDir.z));

  vec3 normal = getWaterNormal(dist, pass_texcoord);

  float fresnel = fresnelSchlick(viewDir, normal, schlick_r0_water);

  float specularDetailFactor = exp(-3 * (dist/30000));
  float specHardness = mix(SPEC_HARDNESS * 0.4, SPEC_HARDNESS, specularDetailFactor);
  float specular = calcSpecular(viewDir, normal, specHardness);
  specular = mix(specular * 0.5, specular, specularDetailFactor);

  vec3 envColor = vec3(0.65, 0.85, 1.0);
  envColor = mix(envColor, vec3(0.7), 0.6);
  envColor *= smoothstep(-0.1, 0.4, sunDir.z);

  vec3 refractionColor = 0.9 * water_color;
  refractionColor *= calcWaterLight(normal);

  vec3 color = mix(refractionColor, envColor, fresnel);
  color += specular * directLightColor * directLight;

  return color;
}


vec3 getWaterColor(vec3 viewDir, float dist, vec2 coord, float waterDepth, vec3 groundColor, vec3 normal,
                   float shallow_sea_amount, float river_amount)
{
  float extinction_factor = waterDepth * 0.15;
  waterDepth = 1 - pow(1-waterDepth, 2);

  extinction_factor *= 4;

  float ambientLight = 0.3 * smoothstep(-0.5, 0.4, sunDir.z);
  float directLight = 0.8 * smoothstep(-0.1, 0.4, sunDir.z);
  directLight = 1.2;

  vec3 directLightColor = vec3(1.0, 1.0, 0.8);
  vec3 directLightColorLow = vec3(1.0, 0.6, 0.2);
  vec3 ambienColor = vec3(0.95, 0.98, 1.0);

  directLightColor = mix(directLightColorLow, directLightColor, smoothstep(-0.1, 0.4, sunDir.z));


  float specularDetailFactor = exp(-3 * (dist/30000));

  float fresnel = fresnelSchlick(viewDir, normal, schlick_r0_water);
  float specHardness = mix(SPEC_HARDNESS * 0.4, SPEC_HARDNESS, specularDetailFactor);
  float specular = calcSpecular(viewDir, normal, specHardness);
  specular = mix(specular * 0.5, specular, specularDetailFactor);

  vec3 envColor = vec3(0.65, 0.85, 1.0);
//   envColor = mix(envColor, vec3(0.9), 0.4);
  envColor = mix(envColor, vec3(0.7), 0.6);
  envColor *= smoothstep(-0.1, 0.4, sunDir.z);

//   vec3 deepColor = texture2D(sampler_deep_water, coord * 2).xyz;
//   vec3 deepColor = vec3(0.150, 0.225, 0.180);
//   vec3 deepColor = vec3(0.165, 0.264, 0.233);

  vec3 river_color = mix(water_color, vec3(0.63, 0.6, 0.2) * 0.5, 0.3);
  
  river_amount = 0;

  vec3 refractionColor = 0.9 * mix(water_color, river_color, river_amount);
  refractionColor *= calcWaterLight(normal);

//   refractionColor *= 0;

  //extinction
  /////////////////////////////////

//   groundColor *= exp(-waterDepth * 0.15 * vec3(4, 1.2, 1));

//   groundColor *= 1.1;

  vec3 extincion = vec3(4.0, 1.2, 1.0);
//   extincion = mix(extincion, vec3(4.0, 4.0, 7.0) * 1, river_amount);

  groundColor *= 0.95;

  groundColor *= exp(-extinction_factor * extincion);
  
  float visibility_shallow = 1-waterDepth;
  float visibility_deep = pow(visibility_shallow * smoothstep(0.3, 1.0, visibility_shallow), 1);

  float visibility = mix(visibility_deep, visibility_shallow, shallow_sea_amount);

  /////////////////////////////////

  refractionColor = mix(groundColor, refractionColor, 1-visibility);

//   float refractionBrightness = clamp(dot(normalize(normal), sunDir), 0.0, 2.0);
//   refractionColor *= mix(0.9, 1.0, refractionBrightness);
//   refractionColor *= mix(0.5, 1.0, refractionBrightness);

//   refractionColor *= calcWaterLight(normal);

  vec3 color = mix(refractionColor, envColor, fresnel);
  color += specular * directLightColor * directLight;

  return color;
}


void sampleWaterTypeMap(out float types[4], out float weights[4])
{
  float x = pass_type_map_coord.x;
  float y = pass_type_map_coord.y;

  float x0 = floor(x);
  float y0 = floor(y);

  float x1 = x0 + 1.0;
  float y1 = y0 + 1.0;

  float x0_w = abs(x - x1);
  float x1_w = 1.0 - x0_w;

  float y0_w = abs(y - y1);
  float y1_w = 1.0 - y0_w;

  types[0] = texelFetch(sampler_water_type_map, ivec2(x0, y0), 0).x;
  types[1] = texelFetch(sampler_water_type_map, ivec2(x0, y1), 0).x;
  types[2] = texelFetch(sampler_water_type_map, ivec2(x1, y0), 0).x;
  types[3] = texelFetch(sampler_water_type_map, ivec2(x1, y1), 0).x;

  weights[0] = x0_w * y0_w;
  weights[1] = x0_w * y1_w;
  weights[2] = x1_w * y0_w;
  weights[3] = x1_w * y1_w;
}


void sampleWaterType(vec2 pos, out float shallow_sea_amount, out float river_amount)
{
  vec2 typeMapCoords = pos.xy / 200.0;

  float water_types[4];
  float water_types_w[4];
  sampleWaterTypeMap(water_types, water_types_w);

  shallow_sea_amount = 0;
  river_amount = 0;
  
  for (int i = 0; i < 4; i++)
  {
    uint water_type = uint(water_types[i] * 255);

    switch(water_type)
    {
      case 0u:
//         deep_sea_amount += water_types_w[i];
//         debugColor = vec3(1,0,1);
        break;
      case 1u:
// //         sea_amount += water_types_w[i];
        break;
      case 2u:
// //         sea_amount += water_types_w[i];
//         debugColor = vec3(1,0,0);
        break;
      case 3u:
        river_amount += water_types_w[i];
        break;
      case 4u:
        shallow_sea_amount += water_types_w[i];
        break;
      default:
//         debugColor = vec3(float(water_type));
//         debugColor = vec3(1,0,0);
        break;
    }
  }
}


#if ENABLE_BASE_WATER_MAP
float getWaterDepthBase(vec2 pos)
{
  float depth = 1 - texture2D(sampler_water_map_base, (pos - height_map_base_origin) / height_map_base_size_m).x;

  float threshold_noise = clamp(genericNoise(pos * 0.0004), -1, 1);
  float threshold_noise_coarse = clamp(genericNoise(pos * 0.0001), -1, 1);

  float threshold = 0.5;

  threshold -= 0.5 * threshold_noise_coarse;
  threshold += 0.5 * threshold_noise;

  threshold = clamp(threshold, 0.1, 0.9);

  threshold = 0.8;

  depth = smoothstep(threshold, threshold + 0.2, depth);

  return depth;
}
#endif


float getWaterDepth(vec2 pos)
{
#if ENABLE_WATER_MAP
  // tiled
  // vec2 waterMapTablePos = fract((pos.xy + water_map_table_shift) / map_size) * map_size;

  // cropped
  vec2 waterMapTablePos = ((pos.xy + water_map_table_shift) / map_size) * map_size;
  
  vec2 waterMapTableCoords = waterMapTablePos / (water_map_table_size * water_map_chunk_size_m);
  
//   float water_map_index = texelFetch(sampler_water_map_table, waterMapTableCoords, 0).x;
  float water_map_index = texture(sampler_water_map_table, waterMapTableCoords).x;
  
//   bvec2 mirror_water_map_coords = bvec2(floor(waterMapTableCoords) / 2.0);

  vec2 waterMapCoords =
    (fract(((pos.xy + water_map_table_shift) / water_map_chunk_size_m)) * water_map_scale) + water_map_shift;

//   if (mirror_water_map_coords.x)
//     waterMapCoords.x = 1.0 - waterMapCoords.x;
//   if (mirror_water_map_coords.y)
//     waterMapCoords.y = 1.0 - waterMapCoords.y;

  float depth = 1 - texture(sampler_water_map, vec3(waterMapCoords, water_map_index)).x;

#if ENABLE_BASE_MAP
  {
    float detail_map_blend = getDetailMapBlend(pos);
    detail_map_blend = smoothstep(0.7, 1.0, detail_map_blend);

    #if ENABLE_BASE_WATER_MAP
      float base_depth = getWaterDepthBase(pos);
      depth = mix(base_depth, depth, detail_map_blend);
    #else
      depth *= detail_map_blend;
    #endif

  }
#endif

  return depth;
#else
  return texture2D(sampler_water_map_simple, pos.xy / map_size).x;
#endif
}


float getShoreWaveStrength(vec2 pos, float waterDepth, float amount)
{
  float shore_wave_strength = 0;

  shore_wave_strength += sampleShoreWave(pos.xy, waterDepth, shore_wave_scroll.x);
  shore_wave_strength += sampleShoreWave(pos.xy, waterDepth, shore_wave_scroll.x + 0.2);
  shore_wave_strength += sampleShoreWave(pos.xy, waterDepth, shore_wave_scroll.x + 0.7);
  
//   shore_wave_strength += sampleShoreWave(pos.xy, waterDepth, shore_wave_scroll.y);
  shore_wave_strength *= 0.6;
  shore_wave_strength = clamp(shore_wave_strength, 0, 1);
  
  shore_wave_strength = mix(shore_wave_strength * 0.3, shore_wave_strength, amount);

//   float shore_wave_strength2 = texture(sampler_shore_wave, vec2(0, shore_wave_scroll)).x;
//   float shore_wave_strength = texture(sampler_shore_wave, (pos.xy / 300.0) + vec2(0, shore_wave_scroll)).x;

// //   shore_wave_strength = mix(shore_wave_strength2, shore_wave_strength, smoothstep(0.5, 0.7, waterDepth));
//   shore_wave_strength = mix(shore_wave_strength, shore_wave_strength2, 0.7 * (1-smoothstep(0.5, 0.9, waterDepth)));


//   shore_wave_strength += 2 * shore_wave_strength * (1 - smoothstep(0.4, 0.5, waterDepth));
  shore_wave_strength += 3 * shore_wave_strength * (1 - smoothstep(0.4, 0.5, waterDepth));
  shore_wave_strength = clamp(shore_wave_strength, 0, 1);
  shore_wave_strength *= smoothstep(0.4, 0.5, waterDepth);
  shore_wave_strength *= pow(1 - ((clamp(waterDepth - 0.5, 0, 1)) * 2), 3);

  return shore_wave_strength;
}


vec4 applyWater(vec4 color,
  vec3 view_dir,
  float dist,
  float waterDepth,
  vec2 mapCoords,
  vec2 pos,
  float shallow_sea_amount,
  float river_amount,
  float bank_amount)
{
  const float foam_threshold = 0.8;
  const float foam_threshold_smooth = 0.05;

  const float foam_coarse_threshold_smooth = 0.4;
  const float foam_coarse_threshold_min = 0.3;
  const float foam_coarse_threshold_max = 1.0;
  float foam_coarse_threshold = 0.8;

  const float surf_threshold_smooth = 0.05;
  const float surf_threshold_min = 0.7;
  const float surf_threshold_max = 1.0;

#if !LOW_DETAIL
  float foamDetail = texture(sampler_beach, vec3(mapCoords * 40, 0)).a;

  vec4 surfColor = texture(sampler_beach, vec3(mapCoords * 80, 1));
  vec4 foamColor = texture(sampler_beach, vec3(mapCoords * 4, 1));
  float waterline_noise = texture(sampler_beach, vec3(mapCoords * 10, 1)).a;
  float waterline_noise2 = texture(sampler_beach, vec3(mapCoords * 40, 1)).a;
#endif

  float terrain_height = (2 * (1-waterDepth)) - 1;
  if (terrain_height > 0)
  {
    terrain_height *= 0.3;
  }

  float water_level = 0;
  float shore_wave_strength = 0;

#if ENABLE_SHORE_WAVES
  shore_wave_strength = getShoreWaveStrength(pos.xy, waterDepth, shallow_sea_amount);
#endif

#if !LOW_DETAIL
  float foam_strength = shore_wave_strength;

  water_level = mix(0, 0.03, 1 - exp(-5 * 1 * shore_wave_strength));

//   float wetness = smoothstep(0.43, 0.5, waterDepth);
  float wetness_secondary = smoothstep(0.34, 0.42, waterDepth);
  wetness_secondary = mix(wetness_secondary, bank_amount, 1 - shallow_sea_amount);
  wetness_secondary *= 0.7;
//   float wetness_threshold = mix(1.0, 0.2, wetness);
//   wetness = smoothstep(wetness_threshold, wetness_threshold + 0.01, surfColor.a);

  surfColor.a = smoothstep(0.5, 1.0, surfColor.a);

  float foam_noise_threshold = mix(1, 0.4, foam_strength);
  float foam_noise = smoothstep(foam_noise_threshold, foam_noise_threshold + 0.5, surfColor.a);
//   foam_noise *= texture(sampler_beach, vec3(mapCoords * 20, 1)).a;

  foam_strength = foam_noise;
//   foam_strength = clamp(foam_strength, 0.0, 0.3);

  foam_strength *= shallow_sea_amount;

  foamColor.xyz = vec3(1);
//   foamColor.xyz = surfColor.xyz * 1.2;

  water_level += 0.01 * pow(waterline_noise, 2);
  water_level += 0.02 * pow(waterline_noise2, 8);

  water_level *= mix(0.25, 1.0, shallow_sea_amount);
#endif

  waterDepth = water_level - terrain_height;

#if !LOW_DETAIL
  float wave_strength = mix(0.0, 1.0, 1 - exp(-2 * clamp(waterDepth, 0, 1)));
  wave_strength = mix(wave_strength, wave_strength * 0.3, river_amount);
#endif

  vec3 water_normal = getWaterNormal(dist, mapCoords);

#if ENABLE_WAVE_FOAM
  float wave_foam_amount = getFoamAmountWithNoise(mapCoords * 2);
//   wave_foam_amount = pow(wave_foam_amount, 1.25);
  wave_foam_amount *= smoothstep(0.6, 1.0, surfColor.a);
  wave_foam_amount *= sea_roughness;
  wave_foam_amount *= wave_strength;
  wave_foam_amount *= 1 - river_amount;
#endif

#if !LOW_DETAIL
  water_normal.xy *= wave_strength;
  water_normal = normalize(water_normal);
#endif

  float water_alpha = smoothstep(0.0, 0.01, waterDepth);

#if !LOW_DETAIL
  float wetness = smoothstep(-0.02, -0.01, waterDepth);
  wetness = max(wetness, wetness_secondary);
#endif

#if !LOW_DETAIL
  color.xyz = mix(color.xyz, color.xyz * 0.8, wetness);
#endif

  vec3 waterColor = getWaterColor(view_dir, dist, mapCoords, waterDepth, color.xyz, water_normal, shallow_sea_amount, river_amount);


#if ENABLE_WAVE_FOAM
  waterColor = mix(waterColor, vec3(1), wave_foam_amount);
#endif

  color.xyz = mix(color.xyz, waterColor, water_alpha);

#if !LOW_DETAIL
  color.xyz = mix(color.xyz, foamColor.xyz, foam_strength);
#endif

  return color;
}
