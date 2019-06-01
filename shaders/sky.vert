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
varying vec3 passObjectPosWorld;
varying vec3 passObjectPos;
varying vec3 passObjectPosView;
uniform vec3 cameraPosWorld;

uniform vec3 frustum_texture_cameraPosWorld;
varying vec3 frustum_texture_pass_pos_world;

void main(void)
{
  vec4 pos = gl_Vertex;
  pos.xyz += cameraPosWorld;
  passObjectPosWorld = pos.xyz;
  passObjectPos = pos.xyz;

  passObjectPosView = (world2ViewMatrix * pos).xyz;

  gl_Position = projectionMatrixFar * world2ViewMatrix * pos;


  const float dist = 1000 * 1000;
  frustum_texture_pass_pos_world = normalize(gl_Vertex.xyz) * dist + frustum_texture_cameraPosWorld;
}
