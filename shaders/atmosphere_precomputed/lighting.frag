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

#version 330

#define USE_LUMINANCE @use_luminance@

#if USE_LUMINANCE
#define GetSunAndSkyIrradiance GetSunAndSkyIlluminance
#define GetSunIrradiance GetSunIlluminance
#endif

vec3 GetSunAndSkyIrradiance(vec3 p, vec3 normal, vec3 sun_direction, out vec3 sky_irradiance);
vec3 GetSunIrradiance(vec3 point, vec3 sun_direction);
vec3 blend_rnm(vec3 n1, vec3 n2);
vec3 textureColorCorrection(vec3 color);


uniform vec3 sunDir;
uniform vec3 earth_center;
uniform float texture_brightness;

varying vec3 passObjectPos;


void calcLightParams(vec3 normal, out vec3 ambientLightColor, out vec3 directLightColor)
{
  directLightColor = GetSunAndSkyIrradiance(
          passObjectPos - earth_center, normal, sunDir, ambientLightColor);
}


vec3 calcIncomingDirectLight()
{
  return GetSunIrradiance(passObjectPos - earth_center, sunDir);
}


void calcLightParamsWithDetail(vec3 normal, vec3 normal_detail,
    out vec3 ambientLightColor, out vec3 directLightColor)
{
  normal = blend_rnm(normal, normal_detail);

  calcLightParams(normal, ambientLightColor, directLightColor);
}


vec3 calcLightWithSpecular(vec3 input_color, vec3 normal, float shinyness, vec3 specular_amount,
    float direct_scale, float ambient_scale, vec3 viewDir)
{
  specular_amount *= texture_brightness;

  vec3 ambientLightColor;
  vec3 directLightColor;
  calcLightParams(normal, ambientLightColor, directLightColor);

  vec3 light = direct_scale * directLightColor + ambient_scale * ambientLightColor;

  vec3 specular = vec3(0);

  {
    vec3 R = reflect(viewDir, normal);
    vec3 lVec = -sunDir;
    specular_amount *= pow(max(dot(R, lVec), 0.0), shinyness);

    specular = specular_amount * calcIncomingDirectLight();
  }

  return (light * input_color) + specular;
}


vec3 calcWaterEnvColor()
{
  vec3 ambientLightColor;
  vec3 directLightColor;
  calcLightParams(vec3(0,0,1), ambientLightColor, directLightColor);

  vec3 envColor = vec3(0.8, 0.9, 1.0) * 0.6;
  envColor = textureColorCorrection(envColor);
  envColor = envColor * ambientLightColor + envColor * directLightColor;

  return envColor;
}
