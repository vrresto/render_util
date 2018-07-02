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
 * This terrain implementation makes use of the technique described in the paper
 * "Continuous Distance-Dependent Level of Detail for Rendering Heightmaps (CDLOD)"
 * by Filip Strugar <http://www.vertexasylum.com/downloads/cdlod/cdlod_latest.pdf>.
 */

#version 130

#extension GL_ARB_draw_instanced : require

#define DRAW_INSTANCED 1

#if DRAW_INSTANCED
attribute vec4 attrib_pos;
#endif

uniform sampler2D sampler_curvature_map;
uniform sampler2D sampler_terrain_cdlod_height_map;

uniform float max_elevation;
uniform float curvature_map_max_distance;
uniform float planet_radius;

uniform vec3 cameraPosWorld;
uniform mat4 projectionMatrixFar;
uniform mat4 world2ViewMatrix;
uniform mat4 view2WorldMatrix;
uniform bool toggle_lod_morph;
uniform vec2 cdlod_grid_size;

#if !DRAW_INSTANCED
uniform vec2 cdlod_node_pos;
uniform float cdlod_node_scale;
uniform float cdlod_lod_distance;
#endif

// uniform ivec2 typeMapSize;
uniform vec2 map_size;

varying vec3 passObjectPosFlat;
varying vec3 passObjectPos;
varying float vertexHorizontalDist;
varying vec3 passLight;
varying vec3 passNormal;

uniform float terrain_height_offset = 0.0;

vec2 getDiff(float dist, float height)
{
  float u = dist / curvature_map_max_distance;
  float v = height / max_elevation;

//   return texture2D(sampler_curvature_map, vec2(u,v)).xy;
  return texture(sampler_curvature_map, vec2(u,v)).xy;
}


vec2 morphVertex(vec2 gridPos, vec2 vertex, float lod_morph)
{
//   vec2 fracPart = fract(gridPos.xy * cdlod_grid_size.xy * 0.5) * 2.0 / cdlod_grid_size.xy;
//   return vertex.xy - fracPart * g_quadScale.xy * morphK;

  vec2 grid_coords = gridPos * cdlod_grid_size;

  vec2 fp = fract(grid_coords / 2) * 2;;

  return grid_coords - fp * lod_morph;
}

void main(void)
{
  vec3 pos = gl_Vertex.xyz;

  vec2 grid_pos = gl_Vertex.xy / cdlod_grid_size;

#if DRAW_INSTANCED
  float cdlod_lod_distance = attrib_pos.w;

  pos.xy *= attrib_pos.z;
  pos.xy += attrib_pos.xy;
#else
  pos.xy *= cdlod_node_scale;
  pos.xy += cdlod_node_pos;
#endif

//   pos.xy += vec2(gl_InstanceID * 1000);
//   pos.xy += texelFetch(sampler_cdlod_node_pos, gl_InstanceID, 0).xy;
//   pos.xy += texelFetch(sampler_cdlod_node_pos, 0, 0).xy;

//   float node_max_height = 3000; //FIXME
  float node_max_height = 0; //FIXME

  float z_dist = max(0, cameraPosWorld.z - node_max_height);

  float lod_morph = smoothstep(cdlod_lod_distance * 0.7, cdlod_lod_distance, distance(pos, vec3(cameraPosWorld.xy, z_dist)));

  pos.xy = morphVertex(grid_pos, pos.xy, lod_morph);

#if DRAW_INSTANCED
  pos.xy *= attrib_pos.z;
  pos.xy += attrib_pos.xy;
#else
  pos.xy *= cdlod_node_scale;
  pos.xy += cdlod_node_pos;
#endif

  vec2 height_map_coord = pos.xy / map_size;
  height_map_coord.y = 1.0 - height_map_coord.y;

  pos.z = texture2D(sampler_terrain_cdlod_height_map, height_map_coord).x;

  pos.z += terrain_height_offset;

  passObjectPosFlat = pos;

  vec2 pos_xy_camera_relative = pos.xy - cameraPosWorld.xy;
  float horizontalDist = length(pos_xy_camera_relative);

  vertexHorizontalDist = horizontalDist;

// curvature
#if 1
  vec2 view_dir_horizontal = normalize(pos_xy_camera_relative);

  vec2 diff = getDiff(horizontalDist, pos.z);

  float horizontalDistNew = horizontalDist + diff.x;
//   float horizontalDistNew = diff.x;

//   vec2 pos_xy_camera_relative_new = view_dir_horizontal * horizontalDist;
  vec2 pos_xy_camera_relative_new = view_dir_horizontal * horizontalDistNew;

//   vec2 pos_xy_camera_relative_new = pos_xy_camera_relative;
  vec2 pos_xy_new = pos_xy_camera_relative_new + cameraPosWorld.xy;

  pos.xy = pos_xy_new;
  pos.z += diff.y;
//   pos.z = diff.y;

//   pos.z -= planet_radius;
#endif

  passObjectPos = pos;

//   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
//   gl_Position = projectionMatrixFar * gl_ModelViewMatrix * gl_Vertex;
//     gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
//   gl_Position = gl_ProjectionMatrix * world2ViewMatrix * gl_Vertex;
  gl_Position = projectionMatrixFar * world2ViewMatrix * vec4(pos, 1);


  passNormal = gl_Normal.xyz;
//   vec3 sun_direction = normalize(vec3(0.5, 0.5, 0.7));
//   float light = clamp(dot(gl_Normal, sun_direction), 0.0, 1.0);
//   passLight = vec3(light);


//   gl_Position = projectionMatrixFar * gl_Vertex;
//   gl_Position = gl_ProjectionMatrix * gl_Vertex;
}
