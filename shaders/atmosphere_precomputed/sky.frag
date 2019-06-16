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

uniform vec3 cameraPosWorld;
uniform vec3 sunDir;
uniform vec3 earth_center;
uniform vec2 sun_size;

varying vec3 passObjectPosWorld;

#if USE_LUMINANCE
#define GetSolarRadiance GetSolarLuminance
#define GetSkyRadiance GetSkyLuminance
#endif

vec3 GetSolarRadiance();
vec3 GetSkyRadiance(vec3 camera, vec3 view_ray, float shadow_length,
    vec3 sun_direction, out vec3 transmittance);

vec3 toneMap(vec3 color);


vec3 getSkyColor(vec3 view_direction)
{
  float shadow_length = 0;
  vec3 transmittance;
  vec3 radiance = GetSkyRadiance(
      cameraPosWorld - earth_center,
      view_direction, shadow_length, sunDir,
      transmittance);

  // If the view ray intersects the Sun, add the Sun radiance.
  if (dot(view_direction, sunDir) > sun_size.y) {
    radiance = radiance + transmittance * GetSolarRadiance();
  }

  return radiance;
}


void main(void)
{
  vec3 view_dir = normalize(passObjectPosWorld - cameraPosWorld);

  gl_FragColor.xyz = toneMap(getSkyColor(view_dir));
  gl_FragColor.w = 1.0;
}
