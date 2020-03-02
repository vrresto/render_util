#version 430

#define FRUSTUM_TEXTURE_FRAMES_NUM @frustum_texture_frames_num:0@

#if @enable_aerial_perspective@

struct FrustumTextureFrame
{
  vec3 sample_offset;
  sampler3D sampler;
  mat4 world_to_view_matrix;
  mat4 view_to_world_rotation_matrix;
  mat4 projection_matrix;
  vec2 ndc_xy_to_view;
  vec3 camera_pos;
//   float z_near;
//   float z_far;
};


uniform ivec3 frustum_texture_size;
uniform FrustumTextureFrame frustum_texture_frames[FRUSTUM_TEXTURE_FRAMES_NUM];
uniform uint current_frustum_texture_frame;


const float z_near = 1.0;
const float z_far = 1000.0 * 1000.0;


float distance_to_plane(vec3 lineP,
               vec3 lineN,
               vec3 planeN,
               float planeD)
{
    return (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
}


void castRayThroughFrustum(vec3 ray_dir,
    out float dist_to_z_near,
    out float dist_to_z_far)
{
//   float z_near = frustum_texture_frames[frustum_texture_frame].z_near;
//   float z_far = frustum_texture_frames[frustum_texture_frame].z_far;

  dist_to_z_near = distance_to_plane(vec3(0), ray_dir, vec3(0,0,-1), z_near);
  dist_to_z_far = distance_to_plane(vec3(0), ray_dir, vec3(0,0,-1), z_far);
}


void castRayThroughFrustum(vec2 ndc_xy,
    out vec3 ray_dir,
    uint frustum_texture_frame,
    out float dist_to_z_near,
    out float dist_to_z_far)
{
  vec2 ndc_xy_to_view = frustum_texture_frames[frustum_texture_frame].ndc_xy_to_view;

  vec2 view_xy = ndc_xy_to_view * ndc_xy;

  ray_dir = normalize(vec3(view_xy, -1));

  castRayThroughFrustum(ray_dir, dist_to_z_near, dist_to_z_far);
}


const float EXPONENT = 4;
// const float EXPONENT = 1.75;
// const float EXPONENT = 1;

float mapToFrustumTextureZ(float z)
{
  z = clamp(z, 0, 1);

  return pow(z, 1.0 / EXPONENT);
}

float mapFromFrustumTextureZ(float z)
{
  z = clamp(z, 0, 1);

  return pow(z, EXPONENT);
}


vec3 getFogColorFromFrustumTexture(vec2 ndc_xy, vec3 view_pos, in FrustumTextureFrame frame)
{
  // FIXME z-coord also needs to be corrected according to frustum pos xy
  // (also in aerial_perspective.glsl!)

  vec3 sample_offset = frame.sample_offset;

  vec3 frustum_coords = vec3(0);
  frustum_coords.xy = (ndc_xy + vec2(1)) / 2;

  frustum_coords.xy *= frustum_texture_size.xy;
  frustum_coords.xy -= sample_offset.xy;
  frustum_coords.xy /= vec2(frustum_texture_size.xy);

  vec3 ray_dir_view;
  float dist_to_z_near;
  float dist_to_z_far;
  castRayThroughFrustum(normalize(view_pos), dist_to_z_near, dist_to_z_far);

  float frustum_dist = dist_to_z_far - dist_to_z_near;

  float dist = length(view_pos);

  float dist_in_frustum = dist - dist_to_z_near;
  float dist_in_frustum_relative = dist_in_frustum / frustum_dist;


  frustum_coords.z = mapToFrustumTextureZ(clamp(dist_in_frustum_relative, 0, 1));

  frustum_coords.z *= frustum_texture_size.z;
  frustum_coords.z -= sample_offset.z;
  frustum_coords.z /= float(frustum_texture_size.z);

  return texture(frame.sampler, frustum_coords.xyz).xyz;
}


void sampleFrustumTextureFrame(in FrustumTextureFrame frame, vec3 pos_world, out vec4 color, out float weight)
{
  color = vec4(0);
  weight = 1.0;

  vec4 frustum_texture_pos_view = frame.world_to_view_matrix * vec4(pos_world, 1);
  vec4 frustum_texture_pos_clip =
      frame.projection_matrix * frustum_texture_pos_view;
  vec2 frustum_texture_pos_ndc_xy = frustum_texture_pos_clip.xy / frustum_texture_pos_clip.w;

  color.rgb = getFogColorFromFrustumTexture(frustum_texture_pos_ndc_xy,
                                            frustum_texture_pos_view.xyz,
                                            frame);

  if (any(lessThan(frustum_texture_pos_ndc_xy, vec2(-1))) ||
      any(greaterThan(frustum_texture_pos_ndc_xy, vec2(1))))
  {
    weight = 0;
  }

  if (frustum_texture_pos_view.z > 0)
  {
    weight = 0;
  }

}


vec4 sampleFrustumTextureCurrentFrame(vec3 pos_world)
{
  vec4 color;
  float weight;
  sampleFrustumTextureFrame(frustum_texture_frames[current_frustum_texture_frame], pos_world, color, weight);
  return color;
}


vec3 getCurrentFrustumSampleOffset()
{
  return frustum_texture_frames[current_frustum_texture_frame].sample_offset;
}


vec3 getCurrentFrustumTextureCamera()
{
  return frustum_texture_frames[current_frustum_texture_frame].camera_pos;
}


mat4 getCurrentFrustumTextureViewToWorldRotationMatrix()
{
  return frustum_texture_frames[current_frustum_texture_frame].view_to_world_rotation_matrix;
}


vec4 sampleAerialPerpective(vec3 pos_world)
{
  vec4 sum_colors = vec4(0);
  float sum_weights = 0;
  for (uint i = 0; i < FRUSTUM_TEXTURE_FRAMES_NUM; i++)
  {
    vec4 color;
    float weight;
    sampleFrustumTextureFrame(frustum_texture_frames[i], pos_world, color, weight);
    sum_colors += color * weight;
    sum_weights += weight;
  }

  if (sum_weights != 0)
    return sum_colors / sum_weights;
  else
    return vec4(0);
}

#else

void aerial_perspective_dummy() {}

#endif
