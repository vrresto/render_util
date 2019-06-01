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


float mapFromFrustumTextureZ(float z);
void castRayThroughFrustum(vec2 ndc_xy,
    out vec3 ray_dir,
    uint frustum_texture_frame,
    out float dist_to_z_near,
    out float dist_to_z_far);
vec3 getCurrentFrustumSampleOffset();
vec3 getCurrentFrustumTextureCamera();
mat4 getCurrentFrustumTextureViewToWorldRotationMatrix();

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f) uniform image3D img_output;

uniform vec3 earth_center;
uniform float earth_radius;
uniform ivec3 texture_size;

uniform uint current_frustum_texture_frame;

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

    float haze_density_step = calcHazeDensityAtPos(pos);
    haze_dist += haze_density_step * step_size;
    float haze_opacity = 1 - exp(-3.0 * (haze_dist / haze_visibility));

    pixel.rgb = vec3(haze_opacity);

    imageStore(img_output, ivec3(pixel_coords, i), pixel);
  }
}
