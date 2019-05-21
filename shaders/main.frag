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


vec3 calcIncomingDirectLight()
{
  vec3 directLightColor = vec3(1.0, 1.0, 0.95);
  vec3 directLightColorLow = directLightColor * vec3(1.0, 0.9, 0.6);

  directLightColorLow *= 0.8;

  vec3 directLightColorVeryLow = directLightColorLow * vec3(1.0, 0.67, 0.37);

  directLightColor = mix(directLightColorLow, directLightColor, smoothstep(0.0, 0.3, sunDir.z));
  directLightColor = mix(directLightColorVeryLow, directLightColor, smoothstep(-0.02, 0.1, sunDir.z));
  directLightColor *= smoothstep(-0.02, 0.02, sunDir.z);

  return directLightColor;
}



// see http://blog.selfshadow.com/publications/blending-in-detail/
vec3 blend_rnm_(vec3 n1, vec3 n2)
{
  n1 = (n1 + vec3(1)) / 2;
  n2 = (n2 + vec3(1)) / 2;

  vec3 t = n1.xyz*vec3( 2,  2, 2) + vec3(-1, -1,  0);
  vec3 u = n2.xyz*vec3(-2, -2, 2) + vec3( 1,  1, -1);
  vec3 r = t*dot(t, u) - u*t.z;

  return normalize(r);
}

void calcLightParamsWithDetail(vec3 normal, vec3 normal_detail,
    out vec3 ambientLightColor, out vec3 directLightColor)
{
  float directLight = 1.0;
  
  if (gl_FragCoord.x < 900)
    normal = blend_rnm_(normal, normal_detail);
  
  directLight *= clamp(dot(normalize(normal), sunDir), 0.0, 2.0);

//   if (gl_FragCoord.x < 900)
//     directLight *= clamp(dot(normalize(normal_detail), sunDir), 0.0, 2.0);

  directLightColor = calcIncomingDirectLight() * directLight;

  float ambientLight = 0.6 * smoothstep(0.0, 0.3, sunDir.z);
  ambientLightColor = vec3(0.95, 0.98, 1.0);
  ambientLightColor *= 0.6;
  vec3 ambientLightColorLow = ambientLightColor * 0.6;
  ambientLightColor = mix(ambientLightColorLow, ambientLightColor, smoothstep(0.0, 0.3, sunDir.z));
  ambientLightColor *= smoothstep(-0.4, 0.0, sunDir.z);
}


void calcLightParams(vec3 normal, out vec3 ambientLightColor, out vec3 directLightColor)
{
  float directLight = 1.0;
  directLight *= clamp(dot(normalize(normal), sunDir), 0.0, 2.0);

  directLightColor = calcIncomingDirectLight() * directLight;

  float ambientLight = 0.6 * smoothstep(0.0, 0.3, sunDir.z);
  ambientLightColor = vec3(0.95, 0.98, 1.0);
  ambientLightColor *= 0.6;
  vec3 ambientLightColorLow = ambientLightColor * 0.6;
  ambientLightColor = mix(ambientLightColorLow, ambientLightColor, smoothstep(0.0, 0.3, sunDir.z));
  ambientLightColor *= smoothstep(-0.4, 0.0, sunDir.z);
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


vec3 calcLightWithSpecular(vec3 input_color, vec3 normal, float shinyness, vec3 specular_amount,
    float direct_scale, float ambient_scale, vec3 viewDir)
{
  vec3 ambientLightColor;
  vec3 directLightColor;
  calcLightParams(normal, ambientLightColor, directLightColor);

  vec3 light = direct_scale * directLightColor + ambient_scale * ambientLightColor;

  vec3 specular = vec3(0);

  {
    vec3 reflectDir = reflect(-sunDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shinyness);

      vec3 R = reflect(viewDir, normal);
      vec3 lVec = -sunDir;
      spec = pow(max(dot(R, lVec), 0.0), shinyness);

    specular = calcIncomingDirectLight() * spec;

    specular *= specular_amount;
  }

  return (light * input_color) + specular;
}


vec3 saturation(vec3 rgb, float adjustment)
{
    // Algorithm from Chapter 16 of OpenGL Shading Language
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

vec3 applyColorCorrection(vec3 color)
{
  return color;
//   return color * vec3(1.0, 0.98, 0.95);
}

vec3 brightnessContrast(vec3 value, float brightness, float contrast)
{
    return (value - 0.5) * contrast + 0.5 + brightness;
}

vec3 textureColorCorrection(vec3 color)
{
  color = brightnessContrast(color, 0.0, 1.1);
  color = saturation(color, 1.05);

  return color;
}
