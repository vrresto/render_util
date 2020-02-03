#version 430


uniform float z_far;
uniform float z_near;
// uniform float fov;
// uniform vec2 viewport_size;
// uniform mat4 ndc_to_view;
uniform vec2 ndc_xy_to_view;
uniform ivec3 frustum_texture_size;

uniform sampler2D sampler_generic_noise;

uniform float frame_phase;


const float fps = 30;

float genericNoise(vec2 pos);


float sampleJitterNoise(vec2 sample_offset, vec3 coords, float freq)
{
  float jitter_noise = 1;

  sample_offset += vec2(frame_phase, 2 * frame_phase);

  coords.x *= 1.5;

  coords.x *= 0.3;
  
  coords *= freq;

  float freq1 = -1 + 2 * texture(sampler_generic_noise, sample_offset + 0.5 * coords.xy).x;
  float freq2 = -1 + 2 * texture(sampler_generic_noise, sample_offset + 1.0 * coords.xy).x;
  float freq3 = -1 + 2 * texture(sampler_generic_noise, sample_offset + 0.25 * coords.xy).x;
//   jitter_noise *= -1 + 2 * texture(sampler_generic_noise, sample_offset + 0.1 * coords.xy).x;

  jitter_noise = freq1;

//   jitter_noise = mix(jitter_noise, freq2, 0.25);
//   jitter_noise = mix(jitter_noise, freq3, 0.2);


  jitter_noise = clamp(jitter_noise, -1, 1);


  return jitter_noise * 0.7 ;
}


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
//   return z;

  z = clamp(z, 0, 1);
  return pow(z, 1.0 / EXPONENT);
}


float mapFromFrustumTextureZ(float z)
{
//   return z;

  z = clamp(z, 0, 1);
  return pow(z, EXPONENT);
}



vec3 getFogColorFromFrustumTexture(vec2 ndc_xy, vec3 view_pos, sampler3D frustum_texture, vec3 pos)
{
//   float noise_scale = 0.000008;
// 
//   float noise1 = texture(sampler_generic_noise, noise_scale * pos.xy).x;
//   float noise2 = texture(sampler_generic_noise, vec2(0.5) + noise_scale * pos.xy).x;
//   float noise3 = texture(sampler_generic_noise, vec2(0.25) + noise_scale * pos.xy).x;
// 
//   vec3 jitter = vec3(noise1, noise2, noise3);
//   jitter -= vec3(0.5);
// //   jitter *= 0.3;
// //   return jitter;

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

  vec2 noise_scale_xy = vec2(0.7);
//   vec3 noise_scale = vec3(noise_scale_xy, 1.0);
  vec3 noise_offset = vec3(0.0, 0.5, 0.0);
  vec3 jitter = vec3(0);
  
  jitter.x = sampleJitterNoise(vec2(0), frustum_coords, 1.0);
  jitter.y = sampleJitterNoise(vec2(0.5), frustum_coords, 1.0);
  jitter.z = sampleJitterNoise(vec2(0.1, 0.2), frustum_coords, 1.0);

//   jitter.xy = vec2(1,0);
  
//   return jitter + vec3(0.5);
  
//   jitter.x = texture(sampler_generic_noise, noise_offset.x + noise_scale_xy * frustum_coords.xy).x;
//   jitter.y = texture(sampler_generic_noise, noise_offset.y + noise_scale_xy * frustum_coords.xy).x;
  
//   jitter.x = texture(sampler_generic_noise, noise_scale.x * frustum_coords.xy).x;
//   jitter.y = texture(sampler_generic_noise, vec2(0.5) + noise_scale.y * frustum_coords.xy).x;
  

//   jitter.z = texture(sampler_generic_noise, vec2(0.2) + noise_scale.z * frustum_coords.xy).x;

//   jitter -= vec3(0.5);

//   jitter.z = 0;
  
//   jitter *= -1;
  
  if (ndc_xy.x < 0)
    jitter *= 0;
  
  
  
  frustum_coords *= vec3(frustum_texture_size);
  
  frustum_coords += jitter;
//   frustum_coords.z += jitter.z;
  
  frustum_coords /= vec3(frustum_texture_size);
  
//   frustum_coords.z = dist / 1000;

  vec3 frustum_color = texture(frustum_texture, frustum_coords.xyz).xyz;
//   float frustum_a = texture(frustum_texture, frustum_coords.xyz).a;

//   frustum_color = texture(frustum_texture, vec3(0.8)).xyz;

  return frustum_color;
}
