/**
 *    Rendering utilities
 *    Copyright (C) 2019 Jan Lepper
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

#include lighting_definitions.glsl

#define MAX_CIRRUS_OPACITY @max_cirrus_opacity@

vec3 fogAndToneMap(vec3 color);
float getSphereIntersectionFromInside(vec3 rayStart, vec3 rayDir, vec3 sphere_center, float radius);
bool sphereIntersection(vec3 ray_origin, vec3 ray_dir, float ray_length,
  vec3 sphere_center, float sphere_radius,
  out float t0, out float t1);

uniform sampler2D sampler_cirrus;
uniform vec3 sunDir;
uniform vec3 cameraPosWorld;
uniform bool is_far_camera = false;
uniform float planet_radius;
uniform float cirrus_height;
uniform float cirrus_layer_thickness;

varying vec3 passObjectPos;
varying vec3 pass_normal;

const float near_dist = 40000;
const float near_fade_dist = 10000;


float getCirrusDensity(vec3 camera_pos, vec3 view_dir);

void main()
{
  vec3 view_dir = normalize(passObjectPos - cameraPosWorld);

  float dist = distance(passObjectPos, cameraPosWorld);

  vec3 normal = pass_normal;
  if (cameraPosWorld.z > cirrus_height)
    normal = -normal;

  float cloud_density = MAX_CIRRUS_OPACITY * getCirrusDensity(cameraPosWorld, view_dir);

  vec3 color = calcCirrusLight(passObjectPos);

  gl_FragColor.w = clamp(cloud_density, 0, 1);

#if 1
  float near_fade = clamp(near_dist - dist, 0, near_fade_dist) / near_fade_dist;
  if (is_far_camera)
    gl_FragColor.w *= 1 - near_fade;
  else
    gl_FragColor.w *= near_fade;
#endif

  gl_FragColor.xyz = color;


  gl_FragColor.xyz = fogAndToneMap(gl_FragColor.xyz);
}
