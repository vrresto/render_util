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

#define ONLY_WATER @enable_water_only:0@
#define ENABLE_UNLIT_OUTPUT @enable_unlit_output:0@

#include water_definitions.glsl

void resetDebugColor();
vec3 getDebugColor();
void getTerrainColor(vec3 pos_curved, vec3 pos_flat, out vec3 lit_color, out vec3 unlit_color);
vec3 getTerrainColor(vec3 pos_curved, vec3 pos_flat);
vec3 fogAndToneMap(vec3);
void fogAndToneMap(in vec3 in_color0, in vec3 in_color1,
                   out vec3 out_color0, out vec3 out_color0);
vec4 sampleAerialPerpective(vec3 pos_world);

layout(location = 0) out vec4 out_color0;
#if ENABLE_UNLIT_OUTPUT
layout(location = 1) out vec4 out_color1;
#endif

uniform float curvature_map_max_distance;

varying float vertexHorizontalDist;
varying vec3 passObjectPosFlat;
varying vec3 passObjectPos;


void main(void)
{
  if (vertexHorizontalDist + 20000.0 > curvature_map_max_distance)
    discard;

  out_color0 = vec4(0.5, 0.5, 0.5, 1.0);
#if ENABLE_UNLIT_OUTPUT
  out_color1 = vec4(0.5, 0.5, 0.5, 1.0);
#endif


  vec3 terrain_color = getTerrainColor(passObjectPos, passObjectPosFlat);
  vec4 aerial_perspective = sampleAerialPerpective(passObjectPos);
  out_color0.rgb = mix(terrain_color, vec3(1), aerial_perspective.r);

//   if (getDebugColor() != vec3(0))
//     out_color0.xyz = getDebugColor();

//   if (int(gl_FragCoord.x) % 8 == 0 && int(gl_FragCoord.y) % 8 == 0)
//     out_color0.xyz = vec3(0, 0.0, 0);

//   out_color0.xyz = vec3(0.0, 0.6, 0.0);
}
