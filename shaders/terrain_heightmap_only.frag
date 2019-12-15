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

#version 330

#include lighting_definitions.glsl

layout(location = 0) out vec4 out_color0;

uniform sampler2D sampler_terrain_cdlod_normal_map;
uniform sampler2D sampler_terrain_cdlod_normal_map_base;

uniform vec3 cameraPosWorld;
uniform vec2 map_size;
uniform int map_resolution_m;

uniform vec2 height_map_base_size_m;
uniform vec2 height_map_base_origin;

varying float vertexHorizontalDist;
varying vec3 passObjectPosFlat;


float getDetailMapBlend(vec2 pos);

vec3 sampleNormalMap(sampler2D sampler, vec2 world_coord, vec2 map_size_world, vec2 map_origin)
{
  vec2 coord = (world_coord.xy - map_origin) / map_size_world;
  coord.y = 1.0 - coord.y;
  vec3 normal = texture2D(sampler, coord).xyz;
  normal.y *= -1;
  return normal;
}

vec3 sampleBaseNormalMap(vec2 world_coord)
{
  return sampleNormalMap(sampler_terrain_cdlod_normal_map_base, world_coord,
                         height_map_base_size_m, height_map_base_origin).xyz;
}


void main(void)
{
  vec2 normal_map_coord = (passObjectPosFlat.xy + vec2(0, map_resolution_m)) / map_size;
  normal_map_coord.y = 1.0 - normal_map_coord.y;

  vec3 normal = texture2D(sampler_terrain_cdlod_normal_map, normal_map_coord).xyz;
  normal.y *= -1;

  float detail_blend = getDetailMapBlend(passObjectPosFlat.xy);

  vec3 base_normal = sampleBaseNormalMap(passObjectPosFlat.xy);

  normal = mix(base_normal, normal, detail_blend);

  vec3 light_direct;
  vec3 light_ambient;
  calcLight(passObjectPosFlat, normal, light_direct, light_ambient);

  out_color0 = vec4(0.5, 0.5, 0.5, 1.0);

  vec3 color_base = vec3(0.05, 0.3, 0.6) * 0.8;
  vec3 color0 = vec3(0.1, 0.5, 0.2) * 0.9;
  vec3 color1 = vec3(0.3, 0.5, 0);
  vec3 color2 = vec3(0.5, 0.5, 0.0);
  vec3 color3 = vec3(1,1,1);

  out_color0.rgb = mix(color_base, color0, min(passObjectPosFlat.z / 2, 1));
  out_color0.rgb = mix(out_color0.rgb, color1, min(passObjectPosFlat.z / 10, 1));
  out_color0.rgb = mix(out_color0.rgb, color2, min(passObjectPosFlat.z / 300, 1));
  out_color0.rgb = mix(out_color0.rgb, color3, min(passObjectPosFlat.z / 2000, 1));

  out_color0.rgb *= (light_direct * 1.5 + light_ambient * 0.5);
}
