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

#define ENABLE_TERRAIN_NORMAL_MAP 1

vec3 calcLight(vec3 pos, vec3 normal);
void apply_fog();

uniform sampler2D sampler_terrain_cdlod_normal_map;

uniform vec2 map_size;

varying vec3 passObjectPosFlat;

void main()
{
  gl_FragColor.xyz = vec3(0.5);
  gl_FragColor.w = 1;

  vec3 pos = passObjectPosFlat.xyz;

#if ENABLE_TERRAIN_NORMAL_MAP
  vec2 normal_map_coord = pos.xy / map_size;
  vec3 normal = texture2D(sampler_terrain_cdlod_normal_map, normal_map_coord).xyz;
#else
  vec3 normal = vec3(0,0,1);
#endif

  vec3 light = calcLight(pos, normal); 

  gl_FragColor.xyz *= light;

  apply_fog();
}
