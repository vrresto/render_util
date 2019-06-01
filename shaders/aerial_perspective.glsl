// based on http://antongerdelan.net/opengl/compute.html

#version 430

layout(local_size_x = 1, local_size_y = 1) in;
// layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image3D img_output;

uniform float z_far;
uniform float z_near;
uniform float fov;
uniform vec2 viewport_size;
uniform vec3 cameraPosWorld;
uniform mat4 view_to_world_rotation;


float distance_to_plane(vec3 lineP,
               vec3 lineN,
               vec3 planeN,
               float planeD)
{
    return (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
}


float calcHazeDensityAtHeight(float height)
{
//   return exp(-(height/2000));
  return 1-smoothstep(0, 2000, height);
}


const vec3 texture_size = vec3(128);

void main(void)
{
  // base pixel colour for image
  vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);
  // get index in global work group i.e x,y position
  ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

  vec2 frustum_pos = (2 * (vec2(gl_GlobalInvocationID.xy) / texture_size.xy)) - vec2(1);

  
  const float aspect = viewport_size.x / viewport_size.y;
//   const vec2 half_size = vec2(z_near * tan(fov / 2.0), (z_near * tan(fov / 2.0)) / aspect);
  const vec2 size = vec2(z_near * tan(fov), (z_near * tan(fov)) / aspect);

  vec3 ray_dir_view = normalize(vec3(frustum_pos * (size / 2), z_near));
  const float ray_length = distance_to_plane(vec3(0), ray_dir_view, vec3(0,0,-1), -z_far);
//   const float ray_length = z_far;

  vec3 ray_dir = -(view_to_world_rotation * vec4(ray_dir_view, 0)).xyz;
  
//   float height = float(pixel_coords.y) / 32.0;
//   pixel.r = 1-height;

  const float step_size = (ray_length / texture_size.z);

  for (int i = 0; i < texture_size.z; i++)
  {
    float dist = i * step_size;
    vec3 pos = cameraPosWorld + dist * ray_dir;

    pixel.r = calcHazeDensityAtHeight(max(pos.z, 0));

//     pixel.r = smoothstep(160, 160, pos.z);
//     pixel.r = smoothstep(000, 6000, i * step_size);

//     pixel.r = pos.z;

//     pixel.r = float(i) / texture_size.z;
    
//     pixel.r = float(i) / texture_size.z;


    // output to a specific pixel in the image
    imageStore(img_output, ivec3(pixel_coords, i), pixel);
  }
}
