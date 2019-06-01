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


layout(location = 0) out vec4 out_color0;
#if ENABLE_UNLIT_OUTPUT
layout(location = 1) out vec4 out_color1;
#endif

uniform ivec3 frustum_texture_size;
uniform vec2 viewport_size;

uniform float curvature_map_max_distance;


uniform mat4 frustum_texture_world2ViewMatrix;
uniform mat4 frustum_texture_projectionMatrixFar;


// uniform float z_near = 0;
// uniform float z_far = 2300000;

uniform sampler3D sampler_aerial_perspective;

varying float vertexHorizontalDist;
varying vec3 passObjectPosFlat;
varying vec3 passObjectPos;
varying vec3 passObjectPosView;
varying vec3 pass_position;
uniform vec2 map_size;

uniform mat4 view2WorldMatrix;

// void castRayThroughFrustum(vec2 ndc_xy,
//     out vec3 ray_dir,
//     out float dist_to_z_near,
//     out float dist_to_z_far);
// float mapToFrustumTextureZ(float z);

vec3 getFogColorFromFrustumTexture(vec2 frag_coord_xy, vec3 view_pos, sampler3D frustum_texture);



void main(void)
{
  if (vertexHorizontalDist + 20000.0 > curvature_map_max_distance)
    discard;

  out_color0 = vec4(0.5, 0.5, 0.5, 1.0);
#if ENABLE_UNLIT_OUTPUT
  out_color1 = vec4(0.5, 0.5, 0.5, 1.0);
#endif

  vec4 frustum_texture_pos_view = frustum_texture_world2ViewMatrix * vec4(passObjectPos, 1);
  vec4 frustum_texture_pos_clip = frustum_texture_projectionMatrixFar * frustum_texture_pos_view;
  vec2 frustum_texture_pos_ndc_xy = frustum_texture_pos_clip.xy / frustum_texture_pos_clip.w;

//   resetDebugColor();
// #if ONLY_WATER
//   float dist = distance(cameraPosWorld, passObjectPosFlat);
//   vec3 view_dir = normalize(passObjectPosFlat - cameraPosWorld);
//   vec3 view_dir_curved = normalize(passObjectPos - cameraPosWorld);
//   out_color0.xyz = getWaterColorSimple(passObjectPos, view_dir_curved, dist);
// #else
//   #if ENABLE_UNLIT_OUTPUT
//     getTerrainColor(passObjectPos, passObjectPosFlat, out_color0.xyz, out_color1.xyz);
//   #else
//     out_color0.xyz = getTerrainColor(passObjectPos, passObjectPosFlat);
//   #endif
// #endif

  vec3 terrain_color = getTerrainColor(passObjectPos, passObjectPosFlat);

//   float prev_dist = distance(prev_cameraPosWorld, passObjectPos);


  vec3 frustum_color = getFogColorFromFrustumTexture(frustum_texture_pos_ndc_xy,
    frustum_texture_pos_view.xyz, sampler_aerial_perspective);

//   out_color0.rgb = frustum_color;
  
//   out_color0 *= 0;
//   out_color0.xy = prev_pos_ndc_xy;
//   out_color0.xy = normalize(prev_pos_view.xy);
  
  
//   vec3 pos_view = frustum_color;
//   vec3 pos_world = (view2WorldMatrix * vec4(pos_view, 1)).xyz;
//   vec3 pos_world = frustum_color;
  
//   out_color0.rgb = vec3(pos_world.z / 1000);
//   out_color0.rgb = vec3(frustum_color.xy / map_size, 0);
//   out_color0.rgb = vec3(length(pos_view) / 100000);

  out_color0.rgb = mix(terrain_color, vec3(1), frustum_color.r);
  
  
//   out_color0 *= 0;
// //   out_color0.xy = (ndc.xy + vec2(1)) / 2;
//   out_color0.z = ndc.z;

//   out_color0.r = frustum_a;

//   out_color0.rgb = vec3(frustum_coords.z);

//   if (getDebugColor() != vec3(0))
//     out_color0.xyz = getDebugColor();

//   if (int(gl_FragCoord.x) % 8 == 0 && int(gl_FragCoord.y) % 8 == 0)
//     out_color0.xyz = vec3(0, 0.0, 0);

//   out_color0.xyz = vec3(0.0, 0.6, 0.0);
}
