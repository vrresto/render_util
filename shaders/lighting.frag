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

vec3 blend_rnm(vec3 n1, vec3 n2);


uniform vec3 sunDir;


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


void calcLightParamsWithDetail_(vec3 normal, vec3 normal_detail,
    out vec3 ambientLightColor, out vec3 directLightColor)
{
  float directLight = 1.0;

  normal = blend_rnm(normal, normal_detail);

  directLight *= clamp(dot(normalize(normal), sunDir), 0.0, 2.0);

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


void calcLightParamsWithDetail(vec3 normal, vec3 normal_detail,
    out vec3 ambientLightColor, out vec3 directLightColor)
{
  calcLightParamsWithDetail_(normal, normal_detail, ambientLightColor, directLightColor);
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
