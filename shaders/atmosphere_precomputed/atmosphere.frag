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

varying vec3 passObjectPos;

#if USE_LUMINANCE
#define GetSkyRadianceToPoint GetSkyLuminanceToPoint
#endif

vec3 GetSkyRadianceToPoint(vec3 camera, vec3 point, float shadow_length,
    vec3 sun_direction, out vec3 transmittance);

vec3 toneMap(vec3 color);


void apply_fog()
{
  vec3 view_direction = normalize(passObjectPos - cameraPosWorld);
  vec3 normal = normalize(passObjectPos - earth_center);

  vec3 radiance = gl_FragColor.xyz;

  float shadow_length = 0;
  vec3 transmittance;
  vec3 in_scatter = GetSkyRadianceToPoint(cameraPosWorld - earth_center,
      passObjectPos - earth_center, shadow_length, sunDir, transmittance);

  gl_FragColor.rgb = toneMap(radiance * transmittance + in_scatter);
}
