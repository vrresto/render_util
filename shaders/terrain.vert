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

void apply_fog();
// vec4 calcAtmosphereColor(float dist);
// float getMaxAtmosphereThickness(float height, float sinViewAzimuth);
// float getMaxAtmosphereThickness(float height, vec2 viewDir);
// float getMaxAtmosphereThickness(vec3 cameraPos, vec2 viewDir);
float getMaxAtmosphereThickness(vec2 cameraPos, vec2 viewDir);
float getMaxAtmosphereThickness(vec3 cameraPos, vec3 viewDir);

// const float PI = 3.1415926535897932384626433832795;
// const float radius = 6371.0 * 1000.0;
// const float max_distance = 40500.0 * 1000.0;
// // const float radius = 1737.1 * 1000.0;
// const float radius = 500.0 * 1000.0;
// const float circumference = 2.0 * PI * radius;

uniform sampler2D sampler_curvature_map;

uniform float max_elevation;
uniform float curvature_map_max_distance;
uniform float planet_radius;

uniform vec3 cameraPosWorld;
uniform mat4 projectionMatrixFar;
uniform mat4 world2ViewMatrix;
uniform mat4 view2WorldMatrix;

varying vec3 passViewPos;
varying vec3 passObjectPosFlat;
varying vec3 passObjectPos;
varying float vertexHorizontalDist;
varying vec3 passLight;
varying vec3 passNormal;


uniform float terrain_height_offset = 0.0;

// vec2 rotate(vec2 v, float a) {
//         float s = sin(a);
//         float c = cos(a);
//         mat2 m = mat2(c, -s, s, c);
//         return m * v;
// }

vec2 getDiff(float dist, float height) {
  float u = dist / curvature_map_max_distance;
  float v = height / max_elevation;

//   return texture2D(sampler_curvature_map, vec2(u,v)).xy;
  return texture(sampler_curvature_map, vec2(u,v)).xy;
}

void main(void)
{
  vec3 pos = gl_Vertex.xyz;

//   pos.z = 0;

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

  passViewPos = (world2ViewMatrix * vec4(pos, 1)).xyz;

//   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
//   gl_Position = projectionMatrixFar * gl_ModelViewMatrix * gl_Vertex;
//     gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
//   gl_Position = gl_ProjectionMatrix * world2ViewMatrix * gl_Vertex;
  gl_Position = projectionMatrixFar * world2ViewMatrix * vec4(pos, 1);


//   passNormal = gl_Normal.xyz;
  passNormal = vec3(0,0,1);
  
//   vec3 sun_direction = normalize(vec3(0.5, 0.5, 0.7));
//   float light = clamp(dot(gl_Normal, sun_direction), 0.0, 1.0);
//   passLight = vec3(light);


//   gl_Position = projectionMatrixFar * gl_Vertex;
//   gl_Position = gl_ProjectionMatrix * gl_Vertex;
}
