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

#define IS_EDITOR @is_editor@
#define ONLY_WATER @enable_water_only:0@
#define ENABLE_UNLIT_OUTPUT @enable_unlit_output:0@

#include water_definitions.glsl

#include water_definitions.glsl

void resetDebugColor();
vec3 getDebugColor();
void getTerrainColor(vec3 pos, out vec3 lit_color, out vec3 unlit_color);
vec3 getTerrainColor(vec3 pos);
vec3 fogAndToneMap(vec3);
void fogAndToneMap(in vec3 in_color0, in vec3 in_color1,
                   out vec3 out_color0, out vec3 out_color0);


layout(location = 0) out vec4 out_color0;
#if ENABLE_UNLIT_OUTPUT
layout(location = 1) out vec4 out_color1;
#endif

#if IS_EDITOR
uniform vec2 height_map_base_size_m;
uniform vec2 height_map_base_origin;
uniform vec2 cursor_pos_ground;
#endif

uniform float curvature_map_max_distance;
uniform float land_map_meters_per_pixel;
uniform vec3 cameraPosWorld;

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

//   resetDebugColor();
#if ONLY_WATER
  float dist = distance(cameraPosWorld, passObjectPosFlat);
  vec3 view_dir = normalize(passObjectPosFlat - cameraPosWorld);
  out_color0.xyz = getWaterColorSimple(passObjectPos, view_dir, dist); //FIXME
#else
  #if ENABLE_UNLIT_OUTPUT
    getTerrainColor(passObjectPosFlat, out_color0.xyz, out_color1.xyz);
  #else
    out_color0.xyz = getTerrainColor(passObjectPosFlat);
  #endif
#endif

#if IS_EDITOR
  if (passObjectPosFlat.x > cursor_pos_ground.x &&
      passObjectPosFlat.x < cursor_pos_ground.x + land_map_meters_per_pixel &&
      passObjectPosFlat.y > cursor_pos_ground.y &&
      passObjectPosFlat.y < cursor_pos_ground.y + land_map_meters_per_pixel)
  {
    out_color0.x = 1;
  }


  if (passObjectPosFlat.x < height_map_base_origin.x ||
      passObjectPosFlat.y < height_map_base_origin.y ||
      passObjectPosFlat.x > height_map_base_origin.x + height_map_base_size_m.x ||
      passObjectPosFlat.y > height_map_base_origin.y + height_map_base_size_m.y)
  {
    out_color0.x = 0.3;
    out_color0.y = 0.3;
    out_color0.z = 0.3;
  }
#endif

#if ENABLE_UNLIT_OUTPUT
  fogAndToneMap(out_color0.xyz, out_color1.xyz, out_color0.xyz, out_color1.xyz);
#else
  out_color0.xyz = fogAndToneMap(out_color0.xyz);
#endif

//   if (getDebugColor() != vec3(0))
//     out_color0.xyz = getDebugColor();

//   if (int(gl_FragCoord.x) % 8 == 0 && int(gl_FragCoord.y) % 8 == 0)
//     out_color0.xyz = vec3(0, 0.0, 0);

//   out_color0.xyz = vec3(0.0, 0.6, 0.0);
}
