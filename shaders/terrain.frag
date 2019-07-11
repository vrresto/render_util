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

#define IS_EDITOR @is_editor@
#define ONLY_WATER @enable_water_only:0@

void resetDebugColor();
vec3 getDebugColor();
void apply_fog();
vec4 getTerrainColor(vec3 pos);
vec3 getWaterColorSimple(vec3 viewDir, float dist);

uniform float curvature_map_max_distance;

#if IS_EDITOR
uniform vec2 height_map_base_size_m;
uniform vec2 height_map_base_origin;
uniform vec2 cursor_pos_ground;
#endif

uniform float land_map_meters_per_pixel;
uniform vec3 cameraPosWorld;

varying float vertexHorizontalDist;
varying vec3 passObjectPosFlat;


void main(void)
{
  if (vertexHorizontalDist + 20000.0 > curvature_map_max_distance)
    discard;

  gl_FragColor = vec4(0.5, 0.5, 0.5, 1.0);

//   resetDebugColor();
#if ONLY_WATER
  float dist = distance(cameraPosWorld, passObjectPosFlat.xyz);
  vec3 view_dir = normalize(cameraPosWorld - passObjectPosFlat.xyz);
  gl_FragColor.xyz = getWaterColorSimple(view_dir, dist);
#else
  gl_FragColor.xyz = getTerrainColor(passObjectPosFlat.xyz).xyz;
#endif
//   gl_FragColor = vec4(0.5, 0.5, 0.5, 1.0);


#if IS_EDITOR
  if (passObjectPosFlat.x > cursor_pos_ground.x &&
      passObjectPosFlat.x < cursor_pos_ground.x + land_map_meters_per_pixel &&
      passObjectPosFlat.y > cursor_pos_ground.y &&
      passObjectPosFlat.y < cursor_pos_ground.y + land_map_meters_per_pixel)
  {
    gl_FragColor.x = 1;
  }


  if (passObjectPosFlat.x < height_map_base_origin.x ||
      passObjectPosFlat.y < height_map_base_origin.y ||
      passObjectPosFlat.x > height_map_base_origin.x + height_map_base_size_m.x ||
      passObjectPosFlat.y > height_map_base_origin.y + height_map_base_size_m.y)
  {
    gl_FragColor.x = 0.3;
    gl_FragColor.y = 0.3;
    gl_FragColor.z = 0.3;
  }
#endif

  apply_fog();

//   if (getDebugColor() != vec3(0))
//     gl_FragColor.xyz = getDebugColor();

//   if (int(gl_FragCoord.x) % 8 == 0 && int(gl_FragCoord.y) % 8 == 0)
//     gl_FragColor.xyz = vec3(0, 0.0, 0);

//   gl_FragColor.xyz = vec3(0.0, 0.6, 0.0);
}
