/**
 *    Rendering utilities
 *    Copyright (C) 2020 Jan Lepper
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


#version 430

#include definitions.glsl
#include constants.glsl


float mapFromFrustumTextureZ(float z);
void castRayThroughFrustum(vec2 ndc_xy,
    out vec3 ray_dir,
    uint frustum_texture_frame,
    out float dist_to_z_near,
    out float dist_to_z_far);
vec3 getCurrentFrustumSampleOffset();
vec3 getCurrentFrustumTextureCamera();
mat4 getCurrentFrustumTextureViewToWorldRotationMatrix();

void ComputeSingleScatteringIntegrand(
    IN(AtmosphereParameters) atmosphere,
    IN(TransmittanceTexture) transmittance_texture,
    Length r, Number mu, Number mu_s, Number nu, Length d,
    bool ray_r_mu_intersects_ground,
    OUT(DimensionlessSpectrum) rayleigh, OUT(DimensionlessSpectrum) mie);


layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f) uniform image3D img_output;

uniform vec3 earth_center;
uniform float earth_radius;
uniform ivec3 texture_size;
uniform vec3 sunDir;
uniform uint current_frustum_texture_frame;

uniform sampler2D transmittance_texture;
uniform sampler3D scattering_density_texture;


void main(void)
{
  vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);

  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

  vec3 sample_offset = getCurrentFrustumSampleOffset();

  vec2 sample_coords_xy = vec2(pixel_coords) + sample_offset.xy;

  vec2 ndc_xy = (2 * (sample_coords_xy / vec2(texture_size.xy))) - vec2(1);

  vec3 ray_dir_view;
  float dist_to_z_near;
  float dist_to_z_far;
  castRayThroughFrustum(ndc_xy, ray_dir_view, current_frustum_texture_frame,
                        dist_to_z_near, dist_to_z_far);

  const float ray_length = dist_to_z_far - dist_to_z_near;
  const vec3 ray_dir = (getCurrentFrustumTextureViewToWorldRotationMatrix() * vec4(ray_dir_view, 0)).xyz;

  vec3 rayleigh_sum = vec3(0.0);
  vec3 mie_sum = vec3(0.0);
  vec3 multiple_scattering_sum = vec3(0);

  vec3 camera_pos = getCurrentFrustumTextureCamera() - earth_center;
  vec3 view_ray = ray_dir;

  Length r = length(camera_pos);
  Length rmu = dot(camera_pos, view_ray);
  Number mu = rmu / r;
  Number mu_s = dot(camera_pos, sunDir) / r;
  Number nu = dot(view_ray, sunDir);
  bool ray_r_mu_intersects_ground = RayIntersectsGround(ATMOSPHERE, r, mu);

  for (int i = 0; i < texture_size.z; i++)
  {
    float sample_coord_z = float(i);
    sample_coord_z += sample_offset.z;

    float dist =
      dist_to_z_near + mapFromFrustumTextureZ(sample_coord_z / float(texture_size.z)) * ray_length;

    float next_step_dist =
      dist_to_z_near + mapFromFrustumTextureZ((sample_coord_z + 1.0) / float(texture_size.z)) * ray_length;

    float step_dist = next_step_dist - dist;

    {
      float d_i = dist;
      // The Rayleigh and Mie single scattering at the current sample point.
      vec3 rayleigh_i = vec3(0);
      vec3 mie_i = vec3(0);

      ComputeSingleScatteringIntegrand(ATMOSPHERE, transmittance_texture,
          r, mu, mu_s, nu, d_i, ray_r_mu_intersects_ground, rayleigh_i, mie_i);

      // Sample weight (from the trapezoidal rule).
//       float weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
      float weight_i = 1;

      rayleigh_sum += rayleigh_i * weight_i * step_dist;
      mie_sum += mie_i * weight_i * step_dist;

      // multiple scattering
      #if 1
      {
        // The r, mu and mu_s parameters at the current integration point (see the
        // single scattering section for a detailed explanation).
        Length r_i =
            ClampRadius(ATMOSPHERE, sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r));
        Number mu_i = ClampCosine((r * mu + d_i) / r_i);
        Number mu_s_i = ClampCosine((r * mu_s + d_i * nu) / r_i);

        vec3 multiple_scattering_i = GetScattering(ATMOSPHERE,
            scattering_density_texture, r_i, mu_i, mu_s_i, nu,
            ray_r_mu_intersects_ground);

        vec3 transmittance = GetTransmittance(ATMOSPHERE,
            transmittance_texture, r, mu, d_i,
            ray_r_mu_intersects_ground);

        multiple_scattering_i *= transmittance;
        multiple_scattering_i *= step_dist * weight_i;

        multiple_scattering_sum += multiple_scattering_i;
      }
      #endif

    }

    vec3 rayleigh = rayleigh_sum * ATMOSPHERE.solar_irradiance *
        ATMOSPHERE.rayleigh_scattering;
    vec3 mie = mie_sum * ATMOSPHERE.solar_irradiance * ATMOSPHERE.mie_scattering;

    rayleigh *= RayleighPhaseFunction(nu);
    mie *= MiePhaseFunction(ATMOSPHERE.mie_phase_function_g, nu);

    vec3 scattering = rayleigh + mie + multiple_scattering_sum;

    pixel.rgb = scattering * SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;

    imageStore(img_output, ivec3(pixel_coords, i), pixel);
  }
}
