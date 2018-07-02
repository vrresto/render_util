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

void resetDebugColor();
vec3 getDebugColor();
void apply_fog();
vec4 getTerrainColor(vec3 pos);

uniform float curvature_map_max_distance;

varying float vertexHorizontalDist;
varying vec3 passObjectPosFlat;


void main(void)
{
  if (vertexHorizontalDist + 20000.0 > curvature_map_max_distance)
    discard;

  gl_FragColor = vec4(0.5, 0.5, 0.5, 1.0);

//   resetDebugColor();

  gl_FragColor = getTerrainColor(passObjectPosFlat.xyz);
  
//   gl_FragColor = vec4(0.5, 0.5, 0.5, 1.0);

  apply_fog();

//   if (getDebugColor() != vec3(0))
//     gl_FragColor.xyz = getDebugColor();

//   if (int(gl_FragCoord.x) % 8 == 0 && int(gl_FragCoord.y) % 8 == 0)
//     gl_FragColor.xyz = vec3(0, 0.0, 0);

//   gl_FragColor.xyz = vec3(0.0, 0.6, 0.0);
}
