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

#define USE_UNCHARTED2_TONE_MAPPING @use_uncharted2_tone_mapping:0@
#define USE_REINHARD_TONE_MAPPING @use_reinhard_tone_mapping:0@
#define USE_DEFAULT_TONE_MAPPING @use_default_tone_mapping:0@

uniform vec3 white_point;
uniform float gamma;
uniform float exposure;
uniform float saturation;
uniform float brightness_curve_exponent;
uniform float texture_brightness;
uniform float texture_brightness_curve_exponent;
uniform float texture_saturation;

uniform float uncharted2_a = 0.15;
uniform float uncharted2_b = 0.50;
uniform float uncharted2_c = 0.10;
uniform float uncharted2_d = 0.20;
uniform float uncharted2_e = 0.02;
uniform float uncharted2_f = 0.30;
uniform float uncharted2_w = 11.2;


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


#if USE_UNCHARTED2_TONE_MAPPING
vec3 uncharted2Tonemap(const vec3 x)
{
  float A = uncharted2_a;
  float B = uncharted2_b;
  float C = uncharted2_c;
  float D = uncharted2_d;
  float E = uncharted2_e;
  float F = uncharted2_f;
  return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 toneMap(vec3 color) {
  color = uncharted2Tonemap(exposure * color);
  vec3 whiteScale = 1.0 / uncharted2Tonemap(white_point * uncharted2_w);

  color *= whiteScale;
  color = pow(color, vec3(1. / gamma));

  color = adjustSaturation(color, saturation);
  return color;
}
#endif


#if USE_REINHARD_TONE_MAPPING
vec3 toneMap(vec3 color)
{
  color *= exposure;
  color = pow(color, vec3(brightness_curve_exponent));
  color = (color/white_point) / (1.0 + (color/white_point));
  color = pow(color, vec3(1.0 / gamma));
  color = adjustSaturation(color, saturation);
  return color;
}
#endif


#if USE_DEFAULT_TONE_MAPPING
vec3 toneMap(vec3 color)
{
  color = pow(color, vec3(brightness_curve_exponent));
  color = pow(vec3(1.0)
          - exp(-color / white_point * pow(exposure, brightness_curve_exponent)), vec3(1.0 / gamma));
  color = adjustSaturation(color, saturation);
  return color;
}
#endif
