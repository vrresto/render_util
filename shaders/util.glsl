/**
 *    Rendering utilities
 *    Copyright (C) 2018  Jan Lepper
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

vec3 debugColor = vec3(0);


// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
bool sphereIntersection(vec3 ray_origin, vec3 ray_dir, float ray_length,
  vec3 sphere_center, float sphere_radius,
  out float t0, out float t1)
{
  vec3 L = sphere_center - ray_origin;
  vec3 D = ray_dir;
  float tCA = dot(L, D);

  if (tCA < 0)
  {
//     debugColor = vec3(1,0,0);
    return false;
  }

  float d = sqrt(dot(L,L) - dot(tCA,tCA));

  if (d < 0)
  {
    debugColor = vec3(0,1,0);
    return false;
  }

  float tHC = sqrt(sphere_radius*sphere_radius - d*d);

  t0 = tCA - tHC;
  t1 = tCA + tHC;

  return true;
}


float getSphereIntersectionFromInside(vec3 rayStart, vec3 rayDir, vec3 sphere_center, float radius)
{
  // scalar projection
  // = distFromCameraToDeepestPoint
  // may be negative
  float rayStartOntoRayDirScalar = dot(sphere_center - rayStart, rayDir);

  if (isnan(rayStartOntoRayDirScalar)) {
    debugColor = vec3(1,0,0);
    return 0.0;
  }

  if (rayStartOntoRayDirScalar < 0) {
//     debugColor = vec3(1,0,1);
//     return 0.0;
  }

  vec3 deepestPoint = rayStart + rayDir * rayStartOntoRayDirScalar;

  float deepestPointHeight = distance(deepestPoint, sphere_center);

  float distFromDeepestPointToIntersection =
    sqrt(pow(radius, 2.0) - deepestPointHeight*deepestPointHeight);

  if (isnan(distFromDeepestPointToIntersection)) {
    debugColor = vec3(1,0,0);
    return 0.0;
  }

  if (distFromDeepestPointToIntersection > rayStartOntoRayDirScalar) {
//     out_color.xyz = vec3(1, 0.5, 0);
//     return -2.0;
  }

  if (distFromDeepestPointToIntersection < 0)
  {
    debugColor = vec3(1,0,1);
    return 0.0;
  }

  float dist = rayStartOntoRayDirScalar + distFromDeepestPointToIntersection;

  return dist;
}
