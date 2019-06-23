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

uniform vec3 white_point;
uniform float gamma;
uniform float exposure;
uniform float saturation;
uniform float brightness_curve_exponent;
uniform float texture_brightness;
uniform float texture_brightness_curve_exponent;
uniform float texture_saturation;


vec3 deGamma(vec3 color)
{
  return pow(color, vec3(gamma));
}


vec3 adjustSaturation(vec3 rgb, float adjustment)
{
  // Algorithm from Chapter 16 of OpenGL Shading Language
  const vec3 W = vec3(0.2125, 0.7154, 0.0721);
  vec3 intensity = vec3(dot(rgb, W));
  return mix(intensity, rgb, adjustment);
}


vec3 textureColorCorrection(vec3 color)
{
  color = adjustSaturation(color, texture_saturation);
  color = deGamma(color);
  color = pow(color, vec3(texture_brightness_curve_exponent));
  color *= texture_brightness;
  return clamp(color, 0, 1);
}


vec3 toneMap(vec3 color)
{
  color = pow(color, vec3(brightness_curve_exponent));
  color = pow(vec3(1.0)
          - exp(-color / white_point * pow(exposure, brightness_curve_exponent)), vec3(1.0 / gamma));
  color = adjustSaturation(color, saturation);
  return color;
}
