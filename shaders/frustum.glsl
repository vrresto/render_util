#version 430


uniform float z_far;
uniform float z_near;
// uniform float fov;
// uniform vec2 viewport_size;
// uniform mat4 ndc_to_view;
uniform vec2 ndc_xy_to_view;

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
    out float dist_to_z_near,
    out float dist_to_z_far)
{
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


vec3 getFogColorFromFrustumTexture(vec2 ndc_xy, vec3 view_pos, sampler3D frustum_texture)
{

  vec3 frustum_coords = vec3(0);
  frustum_coords.xy = (ndc_xy + vec2(1)) / 2;

  vec3 ray_dir_view;
  float dist_to_z_near;
  float dist_to_z_far;
  castRayThroughFrustum(normalize(view_pos), dist_to_z_near, dist_to_z_far);

  float frustum_dist = dist_to_z_far - dist_to_z_near;

  float dist = length(view_pos);

  float dist_in_frustum = dist - dist_to_z_near;
  float dist_in_frustum_relative = dist_in_frustum / frustum_dist;


  frustum_coords.z = mapToFrustumTextureZ(clamp(dist_in_frustum_relative, 0, 1));
  
//   frustum_coords.z = dist / 1000;

  vec3 frustum_color = texture(frustum_texture, frustum_coords.xyz).xyz;
//   float frustum_a = texture(frustum_texture, frustum_coords.xyz).a;

//   frustum_color = texture(frustum_texture, vec3(0.8)).xyz;

  return frustum_color;
}
