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


uniform vec3 sunDir;

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


vec3 calcLight(vec3 pos, vec3 normal)
{

//   vec2 normal_map_coord = pos.xy / (200 * typeMapSize);
//   vec3 normal = texture2D(sampler_terrain_cdlod_normal_map, normal_map_coord).xyz;

  float ambientLight = 0.3 * smoothstep(-0.5, 0.4, sunDir.z);

  float directLight = 0.8 * smoothstep(-0.1, 0.4, sunDir.z);

  directLight = 1.2;

  directLight *= clamp(dot(normalize(normal), sunDir), 0.0, 2.0);

  vec3 directLightColor = vec3(1.0, 1.0, 0.8);
  vec3 directLightColorLow = vec3(1.0, 0.6, 0.2);
  vec3 ambientColor = vec3(0.95, 0.98, 1.0);


//   float light = clamp(directLight + ambientLight, 0, 3);

  directLightColor = mix(directLightColorLow, directLightColor, smoothstep(-0.1, 0.4, sunDir.z));

  vec3 light = directLightColor * directLight + ambientColor * ambientLight;

//   light *= 1.2;
  
  return light;
}


vec3 calcLightWithSpecular(vec3 input, vec3 normal, float shinyness, vec3 specular_amount, vec3 viewDir)
{
  float ambientLight = 0.5 * smoothstep(-0.5, 0.4, sunDir.z);
  float directLight = 1.0;
  directLight *= smoothstep(-0.05, 0.01, sunDir.z);

  directLight *= clamp(dot(normalize(normal), sunDir), 0.0, 2.0);

  vec3 directLightColor = vec3(1.0, 1.0, 0.95);
  vec3 directLightColorLow = vec3(1.0, 0.6, 0.2);
  vec3 ambientColor = vec3(0.95, 0.98, 1.0);

  directLightColor = mix(directLightColorLow, directLightColor, smoothstep(0.0, 0.2, sunDir.z));

  vec3 light = directLightColor * directLight + ambientColor * ambientLight;

  vec3 specular = vec3(0);

  {
    vec3 reflectDir = reflect(-sunDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shinyness);

      vec3 R = reflect(viewDir, normal);
      vec3 lVec = -sunDir;
      spec = pow(max(dot(R, lVec), 0.0), shinyness);

    specular = directLightColor * directLight * spec;

    specular *= specular_amount;
  }

  return light * input + specular;
}


vec3 calcWaterLight(vec3 normal)
{

//   vec2 normal_map_coord = pos.xy / (200 * typeMapSize);
//   vec3 normal = texture2D(sampler_terrain_cdlod_normal_map, normal_map_coord).xyz;

  float ambientLight = 0.6 * smoothstep(-0.5, 0.4, sunDir.z);


  float directLight = 0.7;

  directLight *= clamp(dot(normalize(normal), sunDir), 0.0, 2.0);

  vec3 directLightColor = vec3(1.0, 1.0, 0.8);
  vec3 directLightColorLow = vec3(1.0, 0.6, 0.2);
  vec3 ambientColor = vec3(0.95, 0.98, 1.0);


//   float light = clamp(directLight + ambientLight, 0, 3);

  directLightColor = mix(directLightColorLow, directLightColor, smoothstep(-0.1, 0.4, sunDir.z));

  vec3 light = directLightColor * directLight + ambientColor * ambientLight;

//   light *= 1.2;
  
  return light;
}
