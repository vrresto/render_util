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

vec3 GetSingleScattering(vec3 camera, vec3 view_ray, float dist, vec3 sun_direction);


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

uniform uint current_frustum_texture_frame;

uniform vec3 sunDir;



uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler3D single_mie_scattering_texture;
uniform sampler2D irradiance_texture;

const float haze_visibility = 20000;


float getHeightAtPos(vec3 pos)
{
  return distance(earth_center, pos) - earth_radius;
}


float calcHazeDensityAtHeight(float height)
{
  return exp(-(height/1000));
//   return 1-smoothstep(0, 2000, height);
//   return 1-smoothstep(500, 500, height);
}


float calcHazeDensityAtPos(vec3 pos)
{
  float height = getHeightAtPos(pos);
  return calcHazeDensityAtHeight(height);
}


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

  float haze_dist = 0;
  
//   vec3 single_scattering_sum = vec3(0);

  
  vec3 rayleigh_sum = vec3(0.0);
  vec3 mie_sum = vec3(0.0);

  
  for (int i = 0; i < texture_size.z; i++)
  {
    float sample_coord_z = float(i);
    sample_coord_z += sample_offset.z;

    float dist =
      dist_to_z_near + mapFromFrustumTextureZ(sample_coord_z / float(texture_size.z)) * ray_length;

    float next_step_dist =
      dist_to_z_near + mapFromFrustumTextureZ((sample_coord_z + 1.0) / float(texture_size.z)) * ray_length;

    float step_size = next_step_dist - dist;

    vec3 pos = getCurrentFrustumTextureCamera() + dist * ray_dir;

    vec3 view_ray = ray_dir;
    
    Length r = length(pos-earth_center);
    Length rmu = dot(pos-earth_center, view_ray);
    Number mu = rmu / r;
    Number mu_s = dot(pos-earth_center, sunDir) / r;
    Number nu = dot(view_ray, sunDir);
    bool ray_r_mu_intersects_ground = RayIntersectsGround(ATMOSPHERE, r, mu);

    
//     float haze_density_step = calcHazeDensityAtPos(pos);
//     haze_dist += haze_density_step * step_size;
//     float haze_opacity = 1 - exp(-3.0 * (haze_dist / haze_visibility));
//     pixel.rgb = vec3(haze_opacity);


//     vec3 single_scattering = GetSingleScattering(pos, ray_dir, step_size, sunDir);
#if 1
    
    {
    
      float d_i = dist;
      // The Rayleigh and Mie single scattering at the current sample point.
      vec3 rayleigh_i = vec3(0);
      vec3 mie_i = vec3(0);
#if 1
      ComputeSingleScatteringIntegrand(ATMOSPHERE, transmittance_texture,
          r, mu, mu_s, nu, d_i, ray_r_mu_intersects_ground, rayleigh_i, mie_i);
#endif
      // Sample weight (from the trapezoidal rule).
//       float weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
      float weight_i = 1;

      rayleigh_sum += rayleigh_i * weight_i * step_size;
      mie_sum += mie_i * weight_i * step_size;
    }
    
    
    vec3 rayleigh = rayleigh_sum * ATMOSPHERE.solar_irradiance *
        ATMOSPHERE.rayleigh_scattering;
    vec3 mie = mie_sum * ATMOSPHERE.solar_irradiance * ATMOSPHERE.mie_scattering;

    rayleigh *= RayleighPhaseFunction(nu);
    mie *= MiePhaseFunction(ATMOSPHERE.mie_phase_function_g, nu);
    
    
    vec3 scattering = rayleigh + mie;
    
//     scattering *= 10;
    
    pixel.rgb = scattering * SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
#endif

    imageStore(img_output, ivec3(pixel_coords, i), pixel);
  }
}
