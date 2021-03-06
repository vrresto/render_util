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

#include terrain_params.h.glsl
#include terrain_geometry_util.h.glsl

#define ENABLE_BASE_MAP @enable_base_map@
#define ENABLE_CURVATURE @enable_curvature:1@

attribute vec4 attrib_pos;

uniform sampler2D sampler_curvature_map;

uniform float max_elevation;
uniform float curvature_map_max_distance;

uniform vec3 cameraPosWorld;
uniform vec3 camera_pos_terrain_floor;
uniform vec3 camera_pos_offset_terrain;

uniform mat4 world_to_view_rotation;
uniform mat4 projectionMatrixFar;

uniform float cdlod_min_dist;
uniform float terrain_height_offset = 0.0;
#if @enable_base_map@
uniform float terrain_base_map_height = 0.0;
#endif

uniform Terrain terrain;

varying vec3 passObjectPosFlat;
varying vec3 passObjectPos;
varying vec3 passObjectPosView;
varying float vertexHorizontalDist;
varying vec3 passLight;
varying vec3 passNormal;
varying vec2 pass_texcoord;
varying vec2 pass_type_map_coord;


vec2 getDiff(float dist, float height)
{
  float u = dist / curvature_map_max_distance;
  float v = height / max_elevation;

//   return texture2D(sampler_curvature_map, vec2(u,v)).xy;
  return texture(sampler_curvature_map, vec2(u,v)).xy;
}


float getHeight(vec2 world_coord, float approx_dist)
{
  vec2 height_map_texture_coord = getHeightMapTextureCoords(terrain.detail_layer, world_coord);

  ivec2 height_map_texture_coord_px =
    ivec2(terrain.detail_layer.height_map.size_px * height_map_texture_coord);

  float hm_smoothed = texture2D(terrain.detail_layer.height_map.sampler, height_map_texture_coord).x;
  float hm_raw = texelFetch(terrain.detail_layer.height_map.sampler, height_map_texture_coord_px, 0).x;

  float detail = mix(hm_raw, hm_smoothed,
    smoothstep(cdlod_min_dist / 4.0, cdlod_min_dist / 2.0, approx_dist));

#if ENABLE_BASE_MAP
  vec2 base_height_map_texture_coord = getHeightMapTextureCoords(terrain.base_layer, world_coord);

  float base = texture2D(terrain.base_layer.height_map.sampler, base_height_map_texture_coord).x;
  base += terrain_base_map_height;

  float detail_blend = getDetailMapBlend(world_coord);

  return mix(base, detail, detail_blend);
#else
  return detail;
#endif
}


vec2 morphVertex(vec2 grid_coords, float lod_morph)
{
  vec2 fp = fract(grid_coords / 2) * 2;;

  return grid_coords - fp * lod_morph;
}


void main(void)
{
  float grids_per_tile = terrain.tile_size_m / terrain.mesh_resolution_m;
  float cdlod_lod_distance = attrib_pos.w;

  // float node_max_height = 3000; //FIXME
  float node_max_height = 0; //FIXME
  float z_dist = max(0, cameraPosWorld.z - node_max_height);

  vec2 origin_tile =
    terrain.max_texture_scale *
      floor(camera_pos_terrain_floor.xy / (grids_per_tile * terrain.max_texture_scale));

  float node_scale = attrib_pos.z;

  vec3 pos = gl_Vertex.xyz;

  pos.xy *= node_scale;
  pos.xy += attrib_pos.xy;

  vec2 pos2d_m = pos.xy * terrain.mesh_resolution_m;

  float lod_morph = smoothstep(
            cdlod_lod_distance * 0.7,
            cdlod_lod_distance * 0.95,
            distance(vec3(pos2d_m, 0), vec3(cameraPosWorld.xy, z_dist)));

  pos.xy = morphVertex(gl_Vertex.xy, lod_morph);
  pos.xy *= node_scale;
  pos.xy += attrib_pos.xy;

  pass_texcoord = (pos.xy + vec2(0,1)) / grids_per_tile;
  pass_texcoord -= origin_tile;

  pass_type_map_coord = pos.xy;

  pos2d_m = pos.xy * terrain.mesh_resolution_m;

  float approx_dist = distance(vec3(pos2d_m, 0), vec3(cameraPosWorld.xy, z_dist));

  pos.z = getHeight(pos2d_m, approx_dist);
  pos.z += terrain_height_offset;

  passObjectPosFlat = vec3(pos2d_m, pos.z);

  float height_diff = 0;

  // curvature
  #if ENABLE_CURVATURE
  {
    vec2 pos_xy_camera_relative = pos.xy - camera_pos_terrain_floor.xy;
    pos_xy_camera_relative *= terrain.mesh_resolution_m;
    pos_xy_camera_relative -= camera_pos_offset_terrain.xy;

    float horizontalDist = length(pos_xy_camera_relative);

    vec2 view_dir_horizontal = normalize(pos_xy_camera_relative);

    vec2 diff = getDiff(horizontalDist, pos.z);

    float horizontalDistNew = horizontalDist + diff.x;

    vec2 pos_xy_camera_relative_new = view_dir_horizontal * horizontalDistNew;

    pos_xy_camera_relative_new += camera_pos_offset_terrain.xy;
    pos_xy_camera_relative_new /= terrain.mesh_resolution_m;

    vec2 pos_xy_new = pos_xy_camera_relative_new + camera_pos_terrain_floor.xy;

    pos.xy = pos_xy_new;
    height_diff = diff.y;
  }
  #endif

  passObjectPos = (pos * vec3(vec2(terrain.mesh_resolution_m), 1)) + vec3(0, 0, height_diff);

  pos.xyz -= camera_pos_terrain_floor;
  pos.xy *= terrain.mesh_resolution_m;
  pos.z += height_diff;
  pos.xyz -= camera_pos_offset_terrain;

  gl_Position = world_to_view_rotation * vec4(pos,1);
  passObjectPosView = gl_Position.xyz;
  gl_Position = projectionMatrixFar * gl_Position;

  passNormal = gl_Normal.xyz;
}
