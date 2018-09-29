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

uniform vec2 map_size;

float genericNoise(vec2 coord);

float getDetailMapBlend(vec2 pos)
{
  float blend_dist = 18000.0;

  float threshold_noise = clamp(genericNoise(pos * 0.0008), -1, 1);
  float threshold_noise_coarse = clamp(genericNoise(pos * 0.00005), -1, 1);

  float threshold = 0.5;

  threshold -= 0.5 * threshold_noise_coarse;
  threshold += 0.1 * threshold_noise;

  threshold = clamp(threshold, 0.1, 0.9);

  float detail_blend_x =
    smoothstep(0.0, blend_dist, pos.x) -
    smoothstep(map_size.x - blend_dist, map_size.x, pos.x);

  float detail_blend_y =
    smoothstep(0.0, blend_dist, pos.y) -
    smoothstep(map_size.y - blend_dist, map_size.y, pos.y);

  float detail_blend = detail_blend_x * detail_blend_y;


  detail_blend = smoothstep(threshold, threshold + 0.4, detail_blend);


//   detail_blend *= smoothstep(0.5, 0.6, detail_blend);

  return detail_blend;
}
