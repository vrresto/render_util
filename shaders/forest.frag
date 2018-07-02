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

void resetDebugColor();
vec3 getDebugColor();
void apply_fog();
vec4 getTerrainColor(vec3 pos);
vec4 getForestColor(vec2 pos, int layer);
vec3 calcLight(vec3 pos, vec3 normal);

uniform int forest_layer;
uniform vec3 cameraPosWorld;
uniform ivec2 typeMapSize;

varying vec3 passObjectPosFlat;

#if ENABLE_TERRAIN_NORMAL_MAP
uniform sampler2D sampler_terrain_cdlod_normal_map;
#endif

void main(void)
{
  vec3 pos = passObjectPosFlat;

  float dist = distance(cameraPosWorld, pos);

  if (dist > 5000.0)
    discard;

  gl_FragColor = vec4(0);

#if ENABLE_TERRAIN_NORMAL_MAP
  vec2 normal_map_coord = pos.xy / (200 * typeMapSize);
  vec3 normal = texture2D(sampler_terrain_cdlod_normal_map, normal_map_coord).xyz;
#else
//   vec3 normal = passNormal;
  vec3 normal = vec3(0,0,1);
#endif

  vec3 light = calcLight(pos, normal); 


// if (gl_FragCoord.x > 1200)
  gl_FragColor = getForestColor(pos.xy, forest_layer);
  
  gl_FragColor.xyz *= light;


  apply_fog();

//   if (getDebugColor() != vec3(0))
//     gl_FragColor.xyz = getDebugColor();

//   if (int(gl_FragCoord.x) % 8 == 0 && int(gl_FragCoord.y) % 8 == 0)
//     gl_FragColor.xyz = vec3(0, 0.0, 0);

//   gl_FragColor.xyz = vec3(0.0, 0.6, 0.0);

//   gl_FragColor.a = 0;
}
