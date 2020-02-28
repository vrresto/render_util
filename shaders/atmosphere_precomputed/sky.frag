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

vec3 adjustSaturation(vec3 rgb, float adjustment);
vec3 toneMap(vec3 color);
vec3 getSkyRadiance(vec3 camera_pos, vec3 view_direction);
vec3 getSunRadiance(vec3 camera_pos, vec3 view_direction);
vec4 sampleAerialPerpective(vec3 pos_world);

uniform vec3 sunDir;
uniform vec2 sun_size;
uniform vec3 cameraPosWorld;
uniform float blue_saturation;

varying vec3 passObjectPosWorld;


vec3 getSkyColor(vec3 view_direction)
{
//   vec3 radiance = getSkyRadiance(cameraPosWorld, view_direction);
  vec3 radiance = vec3(0);

  vec3 sun_radiance = getSunRadiance(cameraPosWorld, view_direction);

  vec4 aerial_perspective = sampleAerialPerpective(cameraPosWorld + (view_direction * 1000 * 1000));

//   if (gl_FragCoord.x < 900)
  {
    radiance = aerial_perspective.rgb;
    radiance += sun_radiance;
  }

  float blue_ratio = radiance.b / dot(vec3(1), radiance);

  vec3 color = toneMap(radiance);
  color = adjustSaturation(color, mix(1, blue_saturation, blue_ratio));

  return color;
}


void main(void)
{
  vec3 view_dir = normalize(passObjectPosWorld - cameraPosWorld);

  gl_FragColor.xyz = getSkyColor(view_dir);
  gl_FragColor.w = 1.0;
}
