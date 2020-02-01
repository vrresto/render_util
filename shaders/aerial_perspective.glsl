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


float genericNoise(vec2 pos);
float mapFromFrustumTextureZ(float z);
void castRayThroughFrustum(vec2 ndc_xy,
    out vec3 ray_dir,
    out float dist_to_z_near,
    out float dist_to_z_far);


layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f) uniform image3D img_output;

uniform vec3 cameraPosWorld;
uniform vec3 compute_cameraPosWorld;
uniform mat4 compute_view_to_world_rotation;
uniform vec3 earth_center;
uniform float earth_radius;
uniform ivec3 texture_size;
uniform ivec2 pixel_coords_offset = ivec2(0);
uniform ivec2 pixel_coords_multiplier = ivec2(1);


// const float haze_visibility = 20000;
const float haze_visibility = 10000;
// const float haze_visibility = 3000;


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
#if 0
  return calcHazeDensityAtHeight(height);
#else
  float dist = distance(cameraPosWorld, pos);
  float noise = genericNoise(pos.xy * 0.002);


  float density_near = 1-smoothstep(200, 400, height);
//   float density_far = 1-smoothstep(0, 2000, height);
  float density_far = exp(-(height/700));

  density_near = mix(density_near * 0.0, density_near, noise);

  float density = mix(density_near, density_far, smoothstep(0, 40000, dist));

//   density = density_far;

  return density;
#endif
}


void main(void)
{
  vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);

  ivec2 pixel_coords = (ivec2(gl_GlobalInvocationID.xy) * pixel_coords_multiplier)
      + pixel_coords_offset;

  vec2 ndc_xy = (2 * ((vec2(pixel_coords) + vec2(0.5)) / texture_size.xy)) - vec2(1);

  vec3 ray_dir_view;
  float dist_to_z_near;
  float dist_to_z_far;
  castRayThroughFrustum(ndc_xy, ray_dir_view, dist_to_z_near, dist_to_z_far);

  const float ray_length = dist_to_z_far - dist_to_z_near;
  const vec3 ray_dir = (compute_view_to_world_rotation * vec4(ray_dir_view, 0)).xyz;

  float haze_dist = 0;

  for (int i = 0; i < texture_size.z; i++)
  {
    float dist =
      dist_to_z_near + mapFromFrustumTextureZ(float(i) / float(texture_size.z)) * ray_length;

    float next_step_dist =
      dist_to_z_near + mapFromFrustumTextureZ(float(i+1) / float(texture_size.z)) * ray_length;

    float step_size = next_step_dist - dist;

    vec3 pos = compute_cameraPosWorld + dist * ray_dir;

    float haze_density_step = calcHazeDensityAtPos(pos);
    haze_dist += haze_density_step * step_size;
    float haze_opacity = 1 - exp(-3.0 * (haze_dist / haze_visibility));

    pixel.rgb = vec3(haze_opacity);

    imageStore(img_output, ivec3(pixel_coords, i), pixel);
  }
}
