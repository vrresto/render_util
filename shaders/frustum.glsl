#version 430

struct FrustumTextureFrame
{
  vec3 sample_offset;
  sampler3D sampler;
  mat4 world_to_view_matrix;
  mat4 view_to_world_rotation_matrix;
  mat4 projection_matrix;
  vec2 ndc_xy_to_view;
  vec3 camera_pos;
};


uniform float z_far;
uniform float z_near;
// uniform float fov;
// uniform vec2 viewport_size;
// uniform mat4 ndc_to_view;
// uniform vec2 ndc_xy_to_view;

vec2 getCurrentFrustumFrameNDCToView();

uniform FrustumTextureFrame fustum_texture_frames[@frustum_texture_frames_num@];
uniform uint current_frustum_texture_frame;
uniform ivec3 frustum_texture_size;



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
//   const float aspect = viewport_size.x / viewport_size.y;
//   const vec2 half_size = vec2(z_near * tan(fov / 2.0), (z_near * tan(fov / 2.0)) / aspect);
//   const vec2 size = vec2(z_near * tan(fov), (z_near * tan(fov)) / aspect);

//   ray_dir = normalize(vec3(ndc_xy * (size / 2), z_near));

  dist_to_z_near = distance_to_plane(vec3(0), ray_dir, vec3(0,0,-1), z_near);
  dist_to_z_far = distance_to_plane(vec3(0), ray_dir, vec3(0,0,-1), z_far);
}

void castRayThroughFrustum(vec2 ndc_xy,
    out vec3 ray_dir,
    uint frustum_texture_frame,
    out float dist_to_z_near,
    out float dist_to_z_far)
{
  vec2 ndc_xy_to_view = fustum_texture_frames[frustum_texture_frame].ndc_xy_to_view;

  vec2 view_xy = ndc_xy_to_view * ndc_xy;


//   vec3 pos_ndc = vec3(ndc_xy, 0.0);
  
//   vec3 pos_view = (ndc_to_view * vec4(pos_ndc, 1)).xyz;

//   const float aspect = viewport_size.x / viewport_size.y;
//   const vec2 half_size = vec2(z_near * tan(fov / 2.0), (z_near * tan(fov / 2.0)) / aspect);
//   const vec2 size = vec2(z_near * tan(fov), (z_near * tan(fov)) / aspect);

  ray_dir = normalize(vec3(view_xy, -1));
  
  castRayThroughFrustum(ray_dir, dist_to_z_near, dist_to_z_far);

//   dist_to_z_near = distance_to_plane(vec3(0), ray_dir, vec3(0,0,-1), -z_near);
//   dist_to_z_far = distance_to_plane(vec3(0), ray_dir, vec3(0,0,-1), -z_far);
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


#if 1
vec3 getFogColorFromFrustumTexture(vec2 ndc_xy, vec3 view_pos,
    sampler3D frustum_texture, vec3 sample_offset)
{
  // FIXME needs to take sample offset z into account

  // FIXME z-coord also needs to be corrected according zo frustum pos xy
  // (also in aerial_perspective.glsl!)

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
  
//   frustum_coords.z = dist / 1000;

  vec3 frustum_color = texture(frustum_texture, frustum_coords.xyz).xyz;
//   float frustum_a = texture(frustum_texture, frustum_coords.xyz).a;

//   frustum_color = texture(frustum_texture, vec3(0.8)).xyz;

  return frustum_color;
}
#endif


void sampleFrustumTextureFrame(uint i, vec3 pos_world, out vec4 color, out float weight)
{
  color = vec4(0);
  weight = 1.0;

  vec4 frustum_texture_pos_view = fustum_texture_frames[i].world_to_view_matrix * vec4(pos_world, 1);
  vec4 frustum_texture_pos_clip =
      fustum_texture_frames[i].projection_matrix * frustum_texture_pos_view;
  vec2 frustum_texture_pos_ndc_xy = frustum_texture_pos_clip.xy / frustum_texture_pos_clip.w;

  color.rgb = getFogColorFromFrustumTexture(frustum_texture_pos_ndc_xy,
                                            frustum_texture_pos_view.xyz,
                                            fustum_texture_frames[i].sampler,
                                            fustum_texture_frames[i].sample_offset);

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

vec4 sampleFrustumTexture(vec3 pos_world)
{
#if 1
  vec4 sum_colors = vec4(0);
  float sum_weights = 0;
  for (uint i = 0; i < @frustum_texture_frames_num@; i++)
  {
    vec4 color;
    float weight;
    sampleFrustumTextureFrame(i, pos_world, color, weight);
    sum_colors += color * weight;
    sum_weights += weight;
  }
  return sum_colors / sum_weights;
#else
  vec4 color;
  float weight;
  sampleFrustumTextureFrame(current_frustum_texture_frame, pos_world, color, weight);
  return color;
#endif
}


vec4 sampleFrustumTextureCurrentFrame(vec3 pos_world)
{
  vec4 color;
  float weight;
  sampleFrustumTextureFrame(current_frustum_texture_frame, pos_world, color, weight);
  return color;
}



vec3 getCurrentFrustumSampleOffset()
{
  return fustum_texture_frames[current_frustum_texture_frame].sample_offset;
}

vec3 getCurrentFrustumTextureCamera()
{
  return fustum_texture_frames[current_frustum_texture_frame].camera_pos;
}


mat4 getCurrentFrustumTextureViewToWorldRotationMatrix()
{
  return fustum_texture_frames[current_frustum_texture_frame].view_to_world_rotation_matrix;
}
