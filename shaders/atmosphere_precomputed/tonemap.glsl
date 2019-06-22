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
uniform float texture_brightness;


vec3 textureColorCorrection(vec3 color)
{
  return pow(color, vec3(gamma)) * texture_brightness;
}


vec3 toneMap(vec3 color)
{
  return pow(vec3(1.0) - exp(-color / white_point * exposure), vec3(1.0 / gamma));
}

vec3 deGamma(vec3 color)
{
  return pow(color, vec3(gamma));
}
