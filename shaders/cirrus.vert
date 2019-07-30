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

uniform mat4 projectionMatrixFar;
uniform mat4 world2ViewMatrix;
uniform mat4 view2WorldMatrix;
uniform vec3 cameraPosWorld;
uniform float planet_radius;
uniform vec2 ndc_to_view;
uniform float cirrus_height;
uniform float cirrus_layer_thickness;
uniform bool inside_cirrus=true;

varying vec3 passViewPos;
varying vec3 passObjectPos;
varying vec3 passObjectPosFlat;
varying vec3 pass_normal;

void main(void)
{

  if (inside_cirrus)
  {
    const float dist = 100;
    gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);
    passViewPos.xy = gl_Vertex.xy * ndc_to_view * dist;
    passViewPos.z = -dist;
    passObjectPos = (view2WorldMatrix * vec4(passViewPos, 1)).xyz;
    passObjectPosFlat = passObjectPos;
    return;
  }

  vec4 pos = gl_Vertex;
  pass_normal = gl_Normal.xyz;

  pos.xyz *= planet_radius + cirrus_height +
    (cameraPosWorld.z < cirrus_height ? -(cirrus_layer_thickness/2) : (cirrus_layer_thickness/2));
  pos.z -= planet_radius;
  pos.xy += cameraPosWorld.xy;

  passObjectPos = pos.xyz;
  passObjectPosFlat = vec3(passObjectPos.xy, cirrus_height);
  passViewPos = (world2ViewMatrix * pos).xyz;

  gl_Position = projectionMatrixFar * world2ViewMatrix * pos;
}
