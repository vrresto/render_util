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
 */

#version 130


void calcLightParams(vec3 normal, out vec3 ambientLightColor, out vec3 directLightColor);
void calcLightParamsWithDetail(vec3 normal, vec3 normal_detail,
    out vec3 ambientLightColor, out vec3 directLightColor);

uniform vec3 sunDir;
uniform vec3 cameraPosWorld;


const float PI = acos(-1.0);


// uniform ivec2 typeMapSize;


// vec2 cropCoords(vec2 coords, float crop_amount)
// {
//   float scale = 1 - 2  * crop_amount;
// 
//   coords = fract(coords);
//   coords *= scale;
//   coords += crop_amount;
// 
//   return coords;
// }

// vec3 intersect(vec3 lineP,
//                vec3 lineN,
//                vec3 planeN,
//                float  planeD)
// {
//   float distance = (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
//   return lineP + lineN * distance;
// }


vec2 rotate(vec2 v, float a)
{
  float sn = sin(a);
  float cs = cos(a);

  float px = v.x * cs - v.y * sn; 
  float py = v.x * sn + v.y * cs;

  return vec2(px, py);
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


vec3 calcLight(vec3 pos, vec3 normal, float direct_scale, float ambient_scale)
{
  vec3 ambientLightColor;
  vec3 directLightColor;
  calcLightParams(normal, ambientLightColor, directLightColor);

  vec3 light = direct_scale * directLightColor + ambient_scale * ambientLightColor;

  return light;
}


vec3 calcLightWithDetail(vec3 normal, vec3 normal_detail, float direct_scale, float ambient_scale)
{
  vec3 ambientLightColor;
  vec3 directLightColor;
  calcLightParamsWithDetail(normal, normal_detail, ambientLightColor, directLightColor);

  vec3 light = direct_scale * directLightColor + ambient_scale * ambientLightColor;

  return light;
}


vec3 adjustSaturation(vec3 rgb, float adjustment)
{
    // Algorithm from Chapter 16 of OpenGL Shading Language
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}


vec3 brightnessContrast(vec3 value, float brightness, float contrast)
{
    return (value - 0.5) * contrast + 0.5 + brightness;
}
