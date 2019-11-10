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

#include lighting_definitions.glsl

#define MAX_CIRRUS_ALBEDO @max_cirrus_albedo@
#define USE_LUMINANCE @use_luminance@

#if USE_LUMINANCE
#define GetSunAndSkyIrradiance GetSunAndSkyIlluminance
#define GetSkyRadiance GetSkyLuminance
#define GetSolarRadiance GetSolarLuminance
#endif

vec3 blend_rnm(vec3 n1, vec3 n2);
vec3 textureColorCorrection(vec3 color);

void GetSunAndSkyIrradiance(vec3 pos, vec3 sun_direction,
                            out vec3 sun_irradiance, out vec3 sky_irradiance);
vec3 GetSolarRadiance();
vec3 GetSkyRadiance(vec3 camera, vec3 view_ray, float shadow_length,
    vec3 sun_direction, out vec3 transmittance);


uniform vec3 sunDir;
uniform vec2 sun_size;
uniform vec3 earth_center;


vec3 calcCirrusLight(vec3 pos)
{
  vec3 ambient;
  vec3 direct;
  getIncomingLight(pos, ambient, direct);

  return MAX_CIRRUS_ALBEDO * (direct + ambient);
}


vec3 getSkyRadiance(vec3 camera_pos, vec3 view_direction)
{
  float shadow_length = 0;
  vec3 transmittance;
  vec3 radiance = GetSkyRadiance(
      camera_pos - earth_center,
      view_direction, shadow_length, sunDir,
      transmittance);

  // If the view ray intersects the Sun, add the Sun radiance.
  if (dot(view_direction, sunDir) > sun_size.y) {
    radiance = radiance + transmittance * GetSolarRadiance();
  }

  return radiance;
}


vec3 getSkyColor(vec3 camera_pos, vec3 view_direction)
{
  return getSkyRadiance(camera_pos, view_direction);
}


vec3 getReflectedDirectLight(vec3 normal, vec3 incoming)
{
  return incoming * max(dot(normal, sunDir), 0.0);
}


vec3 getReflectedAmbientLight(vec3 normal, vec3 incoming)
{
//   return incoming * mix(0.5 * (1.0 + dot(normal, point) / r), 1.0, 0.5);
  return incoming * mix(0.5 * (1.0 + dot(normal, vec3(0,0,1))), 1.0, 0.5);
}


void getIncomingLight(vec3 pos, out vec3 ambientLight, out vec3 directLight)
{
  GetSunAndSkyIrradiance(pos - earth_center, sunDir, directLight, ambientLight);
}


vec3 calcLight(vec3 pos, vec3 normal, float direct_scale, float ambient_scale)
{
  vec3 ambient;
  vec3 direct;
  getIncomingLight(pos, ambient, direct);

  ambient = getReflectedAmbientLight(normal, ambient);
  direct = getReflectedDirectLight(normal, direct);

  return direct_scale * direct + ambient_scale * ambient;
}


vec3 calcLightWithDetail(vec3 pos, vec3 normal, vec3 normal_detail,
                         float direct_scale, float ambient_scale)
{
  vec3 ambient;
  vec3 direct;
  getIncomingLight(pos, ambient, direct);

  normal = blend_rnm(normal, normal_detail);

  ambient = getReflectedAmbientLight(normal, ambient);
  direct = getReflectedDirectLight(normal, direct);

  return direct_scale * direct + ambient_scale * ambient;
}


void calcLight(vec3 pos, vec3 normal, out vec3 direct, out vec3 ambient)
{
  getIncomingLight(pos, ambient, direct);

  ambient = getReflectedAmbientLight(normal, ambient);
  direct = getReflectedDirectLight(normal, direct);
}


void calcLightWithDetail(vec3 pos, vec3 normal, vec3 normal_detail, out vec3 direct, out vec3 ambient)
{
  getIncomingLight(pos, ambient, direct);

  normal = blend_rnm(normal, normal_detail);

  ambient = getReflectedAmbientLight(normal, ambient);
  direct = getReflectedDirectLight(normal, direct);
}


vec3 calcWaterEnvColor(vec3 ambientLight, vec3 directLight)
{
  directLight *= mix(max(dot(vec3(0,0,1), sunDir), 0.0), 1.0, 0.5);

  vec3 envColor = vec3(0.8, 0.9, 1.0) * 0.6;
  envColor = textureColorCorrection(envColor);
  envColor = envColor * ambientLight + envColor * directLight;

  return envColor;
}
