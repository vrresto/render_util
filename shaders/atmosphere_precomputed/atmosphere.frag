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

vec3 adjustSaturation(vec3 rgb, float adjustment);

uniform vec3 cameraPosWorld;
uniform vec3 sunDir;
uniform vec3 earth_center;
uniform vec2 sun_size;
uniform float blue_saturation;

varying vec3 passObjectPos;

#if USE_LUMINANCE
#define GetSkyRadianceToPoint GetSkyLuminanceToPoint
#endif

vec3 GetSkyRadianceToPoint(vec3 camera, vec3 point, float shadow_length,
    vec3 sun_direction, out vec3 transmittance);

vec3 toneMap(vec3 color);
vec4 sampleAerialPerpective(vec3 pos_world);

vec3 fogAndToneMap(vec3 in_color, bool no_inscattering)
{
  vec3 view_direction = normalize(passObjectPos - cameraPosWorld);
  float dist = distance(passObjectPos, cameraPosWorld);

  float shadow_length = 0;
  vec3 transmittance = vec3(1);
  vec3 in_scatter = vec3(0);

  vec4 aerial_perspective = sampleAerialPerpective(passObjectPos);

//   vec3 in_scatter = GetSkyRadianceToPoint(cameraPosWorld - earth_center,
//       passObjectPos - earth_center, shadow_length, sunDir, transmittance);


//   transmittance *= 1-aerial_perspective.r;

  in_scatter = aerial_perspective.rgb;
  
//   in_scatter *= 500;

  if (no_inscattering)
    in_scatter = vec3(0);

  vec3 radiance = in_color * transmittance + in_scatter;

  // FIXME HACK to avoid artifacts at close distance
  radiance = mix(in_color, radiance, smoothstep(50, 150, dist));

  float blue_ratio = radiance.b / dot(vec3(1), radiance);

  vec3 color = toneMap(radiance);
  color = adjustSaturation(color, mix(1, blue_saturation, blue_ratio));

  return color;
}


vec3 fogAndToneMap(vec3 in_color)
{
  return fogAndToneMap(in_color, false);
}


vec3 fogAndToneMap(vec3 radiance, vec3 in_scatter, vec3 transmittance)
{
  radiance = radiance * transmittance + in_scatter;

  float blue_ratio = radiance.b / dot(vec3(1), radiance);

  vec3 color = toneMap(radiance);
  color = adjustSaturation(color, mix(1, blue_saturation, blue_ratio));

  return color;
}


// void fogAndToneMap(in vec3 in_color0, in vec3 in_color1,
//                    out vec3 out_color0, out vec3 out_color1)
// {
//   vec3 view_direction = normalize(passObjectPos - cameraPosWorld);
//   vec3 normal = normalize(passObjectPos - earth_center);
// 
//   float shadow_length = 0;
//   vec3 transmittance;
// 
//   vec3 in_scatter = GetSkyRadianceToPoint(cameraPosWorld - earth_center,
//       passObjectPos - earth_center, shadow_length, sunDir, transmittance);
// 
//   out_color0 = fogAndToneMap(in_color0, in_scatter, transmittance);
//   out_color1 = fogAndToneMap(in_color1, in_scatter, transmittance);
// }


vec3 apply_fog(vec3 in_color)
{
  return fogAndToneMap(in_color);
}
