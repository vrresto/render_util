/**
 *    Rendering utilities
 *    Copyright (C) 2019  Jan Lepper
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

#version 130

#include lighting_definitions.glsl

vec3 fogAndToneMap(vec3 color);
float fbm(vec2 x);
float getDistanceToHorizon(float r);
float getSphereIntersectionFromInside(vec3 rayStart, vec3 rayDir, vec3 sphere_center, float radius);
bool sphereIntersection(vec3 ray_origin, vec3 ray_dir, float ray_length,
  vec3 sphere_center, float sphere_radius,
  out float t0, out float t1);

uniform sampler2D sampler_cirrus;
uniform vec3 sunDir;
uniform vec3 cameraPosWorld;
uniform bool is_far_camera = false;
uniform float planet_radius;
uniform float cirrus_height;
uniform bool inside_cirrus=true;
uniform float cirrus_layer_thickness;

varying vec3 passObjectPos;
varying vec3 pass_normal;

const float near_dist = 40000;
const float near_fade_dist = 10000;


//FIXME also in atmosphere.frag
vec3 intersect_(vec3 lineP,
               vec3 lineN,
               vec3 planeN,
               float  planeD,
               float max_dist,
               out float dist)
{
    dist = (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
    if (max_dist != 0)
      dist = min(dist, max_dist);
    return lineP + lineN * dist;
}

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}


// float getCloudDensityNear_old()
// {
//   const int num_layers = 20;
//   const float layer_alpha = 1.0 / num_layers;
// 
//   float cloud_density_near = 0;
// 
//   for (int i = 0; i < num_layers; i++)
//   {
//     float pos_in_layer = float(i) / float(num_layers);
//     pos_in_layer = (2 * pos_in_layer) - 1.0;
// 
//     float density = 1- abs(pos_in_layer);
// 
//     float height = cirrus_height + (pos_in_layer * (cirrus_layer_thickness / 2));
// 
//     vec3 plane_normal = vec3(0,0,1);
// 
//     float dist = 0;
//     vec3 coord = intersect_(cameraPosWorld, normalize(cameraPosWorld-passObjectPos),
//         plane_normal, height, 300, dist);
// 
//     density *= texture2D(sampler_cirrus, coord.xy * 0.00002).x;
// 
//     cloud_density_near += density * layer_alpha;
//   }
// 
//   cloud_density_near *= 1.3;
// 
//   return cloud_density_near;
// }


float getCloudDensityNear(vec3 view_dir)
{
  const int num_layers = 20;
  const float layer_alpha = 1.0 / num_layers;

  float cloud_density_near = 0;

  for (int i = 0; i < num_layers; i++)
  {
    float pos_in_layer = float(i) / float(num_layers);
    pos_in_layer = (2 * pos_in_layer) - 1.0;

    float density = 1 - abs(pos_in_layer);

    float height = cirrus_height + (pos_in_layer * (cirrus_layer_thickness / 2));

    vec3 plane_normal = vec3(0,0,1);
    
//     plane_normal.y = 0.1;
//     plane_normal = normalize(plane_normal);
    
    
    float planeD = height;
    
    if (cameraPosWorld.z < height)
    {
//       plane_normal = -plane_normal;
//       planeD = -planeD;
    }

    vec3 coord = vec3(0);
    float dist = 0;


    #if 0
    {
      const float max_dist = 0;
      coord = intersect_(cameraPosWorld, view_dir, plane_normal, height, max_dist, dist);
    }
    #else
    {
      vec3 sphere_center = vec3(cameraPosWorld.xy, -planet_radius);
      float sphere_radius = planet_radius + height;

      if (cameraPosWorld.z < height)
      {
        float t0 = getSphereIntersectionFromInside(cameraPosWorld, view_dir,
            sphere_center, sphere_radius);
        if (t0 > 0)
        {
          coord = cameraPosWorld + view_dir * t0;
          dist = t0;
        }
      }
      else
      {
        float t0, t1;
        if (sphereIntersection(cameraPosWorld, view_dir, 0, sphere_center, sphere_radius, t0, t1))
        {
          if (t0 > 0)
          {
            coord = cameraPosWorld + view_dir * t0;
            dist = t0;
          }
          else
          {
            coord = cameraPosWorld + view_dir * t1;
            dist = t1;
          }
        }
      }
    }
    #endif

    density *= texture2D(sampler_cirrus, coord.xy * 0.00002).x;

    if (dist > 0)
      cloud_density_near += density * layer_alpha;
  }

  cloud_density_near *= 2;

  cloud_density_near = min(cloud_density_near, 1);

  return cloud_density_near;
}


float getCloudDensityAt(vec3 pos)
{
  float density = texture2D(sampler_cirrus, pos.xy * 0.00002).x;
  density *= 1 - smoothstep(0, cirrus_layer_thickness/2, abs(cirrus_height - pos.z));

  return density;
}

float getCloudDensityLod(vec3 pos, float lod)
{
  float height = pos.z;

//   {
//     pos.xy -= cameraPosWorld.xy;
//     pos.z += planet_radius;
//     height = length(pos) - planet_radius;
//   }
  

  float density = textureLod(sampler_cirrus, pos.xy * 0.00002, lod).x;
  
  
  density *= 1 - smoothstep(0, cirrus_layer_thickness/2, abs(cirrus_height - height));

  return density;
}


float rayMarchDistance(vec3 ray_origin, vec3 ray_dir, float dist)
{
  const int num_steps = 20;
//   float step_size = dist / float(num_steps);

  float density_dist = 0;
  float marched_dist = 0;

  for (int i = 0; i < num_steps; i++)
  {
    float step_dist_relative = float(i) / float(num_steps);
//     step_dist_relative *= smoothstep(-0.5, 0.5, step_dist_relative);

    float step_dist = step_dist_relative * dist;

    vec3 step_pos = ray_origin + ray_dir * step_dist;

    float step_length = step_dist - marched_dist;

    density_dist += step_length * getCloudDensityAt(step_pos);

    marched_dist = step_dist;
  }

  return density_dist;
}


float rayMarchDistance2(vec3 ray_origin, vec3 ray_dir, float step_length, float num_steps)
{
//   const float start_step_length = 500;


  float density_dist = 0;
  float step_dist = 0;
//   float step_length = start_step_length;
  float lod = 0;

  for (int i = 0; i < num_steps; i++)
  {
    step_dist += step_length;
    vec3 step_pos = ray_origin + ray_dir * step_dist;
    
    float density_factor = 1-smoothstep(0, num_steps * step_length, step_dist);

    float density = step_length * getCloudDensityLod(step_pos, lod);
    density *= density_factor;
//     density *= 0.3;
//     density += 0.1;
//     density = mix(0.3, density, density_factor);

    density_dist += density;

//     step_length *= 1.2;
//     lod += 0.1;
  }

  return density_dist;
}


float rayMarch(vec3 ray_origin, vec3 ray_dir)
{
  const float visibility = 3000;
  const float near_dist = 1000;

  float density_dist = 0;

//   density_dist += rayMarchDistance(ray_origin + ray_dir * near_dist, ray_dir, 5000);
//   density_dist += rayMarchDistance(ray_origin, ray_dir, near_dist);
  
//   density_dist += rayMarchDistance2(ray_origin, ray_dir, 20);
  density_dist += rayMarchDistance2(ray_origin, ray_dir, 100, 40);
//   density_dist += rayMarchDistance2(ray_origin, ray_dir, 500, 20);

  float opacity = 1.0 - exp(-3.0 * (density_dist / visibility));

  opacity * 0.6;

  return opacity;
}


float getDeepestPoint(vec3 rayStart, vec3 rayDir, float radius)
{
  // scalar projection
  float rayStartOntoRayDirScalar = dot(-rayStart, rayDir);
  
//   debugColor = vec3(0,0,1);
  
  if (isnan(rayStartOntoRayDirScalar)) {
    return 0.0;
  }
   
  if (rayStartOntoRayDirScalar < 0) {
    return 0.0;
  }

  vec3 deepestPoint = rayStart + rayDir * rayStartOntoRayDirScalar;
  
  float deepestPointHeight = length(deepestPoint) - radius;
  
  return deepestPointHeight;
}

void main()
{
//   vec3 rot_axis = cross(vec3(0,0,1), normalize(passObjectPos + vec3(0,0,planet_radius)));
//   float rotation_angle = acos(dot(vec3(0,0,1), normalize(passObjectPos + vec3(0,0,planet_radius))));
//   mat4 rotation_matrix = rotationMatrix(-rot_axis, rotation_angle);
//   vec3 sun_dir_at_object_pos = (rotation_matrix * vec4(sunDir, 0)).xyz;
//   float deepest_point = getDeepestPoint(passObjectPos + vec3(0,0,planet_radius),
//     sun_dir_at_object_pos, planet_radius);

  vec3 view_dir = normalize(passObjectPos - cameraPosWorld);

  vec3 lightColor = calcCirrusLight(passObjectPos);

//   if (deepest_point > 0)
//   {
//     lightColor *= 1-smoothstep(0, 100, deepest_point);
//   }

  float inside_cirrus_amount =
    smoothstep(cirrus_height - 50 - (cirrus_layer_thickness/2), cirrus_height - 50, cameraPosWorld.z);
  inside_cirrus_amount *=
    1 - smoothstep(cirrus_height + 50, cirrus_height + 50 + (cirrus_layer_thickness/2), cameraPosWorld.z);


//   inside_cirrus_amount = 1-step(100 + cirrus_layer_thickness/2, abs(cirrus_height - cameraPosWorld.z));
  inside_cirrus_amount = 1-smoothstep(cirrus_layer_thickness/2,
      cirrus_layer_thickness/2 + 100,
      abs(cirrus_height - cameraPosWorld.z));

//   inside_cirrus_amount = 0;

#if 1
  if (inside_cirrus)
  {
    gl_FragColor = vec4(1.0);
#if 0
    gl_FragColor.a = texture2D(sampler_cirrus, cameraPosWorld.xy * 0.00002).x;
    gl_FragColor.a *= 0.5;
    gl_FragColor.a *= inside_cirrus_amount;
#else
    gl_FragColor.a = rayMarch(cameraPosWorld, view_dir);
//     gl_FragColor.a = getCloudDensityNear(view_dir);
    gl_FragColor.a *= inside_cirrus_amount;
#endif

    gl_FragColor.xyz = lightColor;
    return;
  }
#endif

  float dist = distance(passObjectPos, cameraPosWorld);

  

  vec3 normal = pass_normal;
  if (cameraPosWorld.z > cirrus_height)
    normal = -normal;

  float cloud_density_near = getCloudDensityNear(view_dir);

#if 1
//   cloud_density_near = mix(rayMarch(passObjectPos, view_dir), cloud_density_near,
//     smoothstep(1000, 1500, dist));

//   cloud_density_near = rayMarch(passObjectPos, view_dir);
#endif

  float cloud_density = 1;

  float continuency = smoothstep(0.3, 0.9, fbm(vec2(1.0, 1.5) * passObjectPos.xy * 0.00001));

  float sharpness = 1.0;
//   sharpness *= smoothstep(0.0, 0.05, dot(normal, view_dir));

  cloud_density *= smoothstep((1-continuency) * 0.6 * sharpness, 1.0 - (1-continuency) * 0.1 * sharpness,
      fbm(vec2(1.0, 1.5) * passObjectPos.xy * 0.00005));
  cloud_density *= 1 - 0.5 * (1-sharpness);
  cloud_density *= 0.5;

  const float near_blend_dist = 30000;
  float near_blend_start = 30000;

  float near_blend_noise = smoothstep(0.45, 0.55, fbm(vec2(1.0, 1.5) * passObjectPos.xy * 0.00001));

  near_blend_start += near_blend_noise * 10000;

  cloud_density = mix(cloud_density_near, cloud_density,
                      smoothstep(near_blend_start,
                                 near_blend_start+near_blend_dist, dist));

  vec3 color = lightColor;


  gl_FragColor.w = clamp(cloud_density, 0, 1);

#if 1
  float near_fade = clamp(near_dist - dist, 0, near_fade_dist) / near_fade_dist;
  if (is_far_camera)
    gl_FragColor.w *= 1 - near_fade;
  else
    gl_FragColor.w *= near_fade;
#endif

  gl_FragColor.xyz = color;


//   gl_FragColor.a *= smoothstep(0.0, 0.02, dot(normal, view_dir));

  gl_FragColor.a *= mix(1, smoothstep(0, 4000, dist), inside_cirrus_amount);

  gl_FragColor.xyz = fogAndToneMap(gl_FragColor.xyz);
  
}
