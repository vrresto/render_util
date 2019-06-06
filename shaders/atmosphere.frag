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

/**
 * This shader uses methods described in the following papers:
 *
 * "Real-Time Rendering of Planets with Atmospheres" by Tobias Schafhitzel, Martin Falk,  Thomas Ertl
 * <https://www.researchgate.net/publication/230708134_Real-Time_Rendering_of_Planets_with_Atmospheres>
 * 
 * "Precomputed Atmospheric Scattering" by Eric Bruneton, and Fabrice Neyret
 * <https://hal.inria.fr/inria-00288758/en>
 */

#version 130

#define ENABLE_FOG 1

vec3 applyColorCorrection(vec3 color);

vec3 debugColor;

uniform sampler2D sampler_atmosphere_thickness_map;
uniform sampler2D sampler_curvature_map;

uniform vec3 sunDir;
uniform float max_elevation;
uniform float curvature_map_max_distance;
uniform float atmosphereHeight = 100.0;
uniform float planet_radius;
uniform mat4 view2WorldMatrix;
uniform mat4 world2ViewMatrix;
uniform vec3 cameraPosWorld;

varying vec3 passObjectPosFlat;
varying vec3 passObjectPos;
varying vec3 passObjectPosView;
varying vec3 passNormal;

// const float atmosphereVisibility = 400000.0;
// const float atmosphereVisibility = 200000.0;
const float atmosphereVisibility = 600000.0;
// const float atmosphereVisibility = 900000.0;

// const float hazyness = 0.0;
// const float hazyness = 0.05;
const float hazyness = 0.0;

const float HAZE_VISIBILITY = 40000;
const float GROUND_FOG_HEIGHT = 300;
const float GROUND_FOG_DENSITY_SCALE = 1;
const float MIE_PHASE_COEFFICIENT = 0.7;

const float PI = acos(-1.0);


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
//     gl_FragColor.xyz = vec3(1, 0.5, 0);
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


float sphericalFogDistance(vec3 ray_origin, vec3 ray_dir, float ray_length, vec3 sphere_center, float sphere_radius)
{
  float t0 = 0;
  float t1 = 0;

  if (distance(ray_origin, sphere_center) < sphere_radius)
  {
    t1 = getSphereIntersectionFromInside(ray_origin, ray_dir, sphere_center, sphere_radius);
  }
  else
  {
    bool intersection = sphereIntersection(ray_origin, ray_dir, ray_length,
      sphere_center, sphere_radius,
      t0, t1);

    if (!intersection)
    {
      return 0.f;
    }

    t0 = max(0, t0);
  }

  float segment_length = min(max(0, t1 - t0), max(0, ray_length - t0));

  return segment_length;
}


vec2 getMaxAtmosphereThickness(vec2 cameraPos, vec2 viewDir);


void resetDebugColor() {
  debugColor = vec3(0);
}

vec3 getDebugColor() {
  return debugColor;
}


float miePhase(vec3 view_dir, vec3 light_dir, float g)
{
  return (1.f - g*g) / (4*PI * pow(1.f + g*g - 2*g*dot(view_dir, light_dir), 1.5));
}


float getAtmosphereEdgeIntersectionDistFromInside(vec2 rayStart, vec2 rayDir)
{
//   rayStart = round(rayStart / 1000.0);

  // scalar projection
  // = distFromCameraToDeepestPoint
  // may be negative
  float rayStartOntoRayDirScalar = dot(-rayStart, rayDir);
  
  if (isnan(rayStartOntoRayDirScalar)) {
    debugColor = vec3(1,0,0);
    return 0.0;
  }
    
  if (rayStartOntoRayDirScalar < 0) {
//     debugColor = vec3(1,0,1);
//     return 0.0;
  }

  vec2 deepestPoint = rayStart + rayDir * rayStartOntoRayDirScalar;

  float deepestPointHeight = length(deepestPoint);

  float distFromDeepestPointToIntersection =
    sqrt(pow(planet_radius + atmosphereHeight, 2.0) - deepestPointHeight*deepestPointHeight);

  if (isnan(distFromDeepestPointToIntersection)) {
    debugColor = vec3(1,0,0);
    return 0.0;
  }

  if (distFromDeepestPointToIntersection > rayStartOntoRayDirScalar) {
//     gl_FragColor.xyz = vec3(1, 0.5, 0);
//     return -2.0;
  }
  
  if (distFromDeepestPointToIntersection < 0)
  {
    debugColor = vec3(1,0,1);
    return 0.0;
  }

  float radius_ray_start = max(0, length(rayStart));

  float min_atmosphere_distance_from_ray_start =
    (planet_radius + atmosphereHeight) - radius_ray_start;

//   {
//     min_atmosphere_distance_from_ray_start -= 1.0; // tolerance
// 
//     if (rayStartOntoRayDirScalar + distFromDeepestPointToIntersection < min_atmosphere_distance_from_ray_start) {
//       debugColor = vec3(1,1,0);
//     }
//   }

//   min_atmosphere_distance_from_ray_start -= 1000;

  float dist = rayStartOntoRayDirScalar + distFromDeepestPointToIntersection;

  dist = max(min_atmosphere_distance_from_ray_start, dist);
  
//   dist += 1000;

  return dist;
}

float getDistanceToHorizon(float r)
{
  if (r == planet_radius)
    return 0.0;
  else
    return sqrt(r*r - planet_radius*planet_radius);
}

float getMaxAtmosphereDistance()
{
  return getDistanceToHorizon(planet_radius + atmosphereHeight);
}



bool intersectsSphereFromOutside(vec3 rayStart, vec3 rayDir, float radius)
{
  // scalar projection
  float rayStartOntoRayDirScalar = dot(-rayStart, rayDir);
  
//   debugColor = vec3(0,0,1);
  
  if (isnan(rayStartOntoRayDirScalar)) {
    debugColor = vec3(1,0,0);
    return false;
  }
   
  if (rayStartOntoRayDirScalar < 0) {
    return false;
  }

  vec3 deepestPoint = rayStart + rayDir * rayStartOntoRayDirScalar;
  
  float deepestPointHeight = length(deepestPoint) - radius;
  
  return deepestPointHeight < 0.0;
}


bool intersectsCircleFromOutside(vec2 cameraPos, vec2 viewDir, float radius)
{
  return intersectsSphereFromOutside(vec3(cameraPos, 0), vec3(viewDir, 0), radius);
}

bool intersectsGround(vec3 rayStart, vec3 rayDir)
{
  return intersectsSphereFromOutside(rayStart, rayDir, planet_radius);
}

float getCircleIntersectionDistFromOutside(vec2 rayStart, vec2 rayDir, float radius)
{
  // scalar projection
  float rayStartOntoRayDirScalar = dot(-rayStart, rayDir);
  
  if (isnan(rayStartOntoRayDirScalar)) {
    debugColor = vec3(1,0,0);
    return 0.0;
  }
    
  if (rayStartOntoRayDirScalar < 0) {
    debugColor = vec3(1,0,1);
    return 0.0;
  }

  vec2 deepestPoint = rayStart + rayDir * rayStartOntoRayDirScalar;

  float deepestPointHeight = length(deepestPoint);

  float distFromDeepestPointToIntersection =
    sqrt(pow(radius, 2.0) - deepestPointHeight*deepestPointHeight);

  if (isnan(distFromDeepestPointToIntersection)) {
    debugColor = vec3(1,0,0);
    return 0.0;
  }

  if (distFromDeepestPointToIntersection > rayStartOntoRayDirScalar) {
    debugColor = vec3(1,0,1);
    return 0.0;
  }


  return (rayStartOntoRayDirScalar - distFromDeepestPointToIntersection);
//   return (rayStartOntoRayDirScalar - distFromDeepestPointToIntersection) * 1000.0;
}


vec2 getMaxAtmosphereThickness(float height, float mu)
{
  float r = planet_radius + height;
  float Rg = planet_radius;
  float Rt = planet_radius + atmosphereHeight;

  float p = sqrt(r*r - Rg*Rg);
  float H = sqrt(Rt*Rt - Rg*Rg);
  
  float Ur = p / H;

//   float relativeHeight = height / atmosphereHeight;

  vec2 t = texture2D(sampler_atmosphere_thickness_map, vec2(mu, Ur)).xy;
  
  if (t.x < 0.0)
    debugColor = vec3(0,1,1);
  
  return t;
//   return 10000.0;
}


vec2 getMaxAtmosphereThickness(vec2 cameraPos, vec2 viewDir)
{
  cameraPos.y += planet_radius;

//   vec2 cameraPos = vec2(0, planet_radius + height);
  float rAtmosphereEdge = planet_radius + atmosphereHeight;


  if (length(cameraPos) > rAtmosphereEdge)
  {
    if (intersectsCircleFromOutside(cameraPos, viewDir, rAtmosphereEdge))
    {
      float atmosphereEdgeIntersectionDist = getCircleIntersectionDistFromOutside(cameraPos, viewDir, planet_radius + atmosphereHeight);

      if (isnan(atmosphereEdgeIntersectionDist))
      {
        debugColor = vec3(1, 0, 0);
        return vec2(0.0);
      }

      if (atmosphereEdgeIntersectionDist < 0)
      {
        debugColor = vec3(1, 0.5, 0);
        return vec2(0.0);
      }

      cameraPos += viewDir * atmosphereEdgeIntersectionDist;
    }
    else
    {
      return vec2(0.0);
    }
  }

  float rCameraPos = length(cameraPos);


//   {
//     float tolerance = 0.5;
//       if (rCameraPos + tolerance < planet_radius) {
//       debugColor = vec3(1,1,0);
//       return 0.0;
//     }
//   }
  rCameraPos = max(planet_radius, rCameraPos); // account for precision errors


  float distToAtmosphereBorder = getAtmosphereEdgeIntersectionDistFromInside(cameraPos, viewDir);

  float dist_to_horizon = getDistanceToHorizon(rCameraPos);
  if (isnan(dist_to_horizon))
    dist_to_horizon = 0.0;

  float minDistToAtmosphereBorder = (planet_radius + atmosphereHeight) - rCameraPos;
  float maxDistToAtmosphereBorder = dist_to_horizon + getMaxAtmosphereDistance();

  if (distToAtmosphereBorder <= 0) {
    debugColor = vec3(1,0,1);
    return vec2(0.0);
  }

//   if (isnan(maxDistToAtmosphereBorder)) {
//     debugColor = vec3(1,0,1);
//     return 0.0;
//   }


  float minMu = minDistToAtmosphereBorder / maxDistToAtmosphereBorder;
  float mu = distToAtmosphereBorder / maxDistToAtmosphereBorder;
  const float maxMu = 1.0;
  mu = (mu - minMu) / (maxMu - minMu); 
  
//   return mu;
  
  
  if (isnan(mu)) {
    debugColor = vec3(1,0,0);
  }

//   float cosViewAngle = dot(cameraPos, viewDir) / rCameraPos;
  
//   if (rCameraPos - planet_radius < -8.0) {
//     return -5.0;
//   }

//   return mu;

  return getMaxAtmosphereThickness(max(0.0, rCameraPos - planet_radius), mu);

//   gl_FragColor.xyz = vec3(0,0,0);
//   if (!isnan(mu))
//   {
//     gl_FragColor.xyz = vec3(mu);
//   }

//   return mu;

}

vec2 getMaxAtmosphereThicknessFromObject(vec3 objectPosWorld)
{
  float horizontalDist = distance(objectPosWorld.xy, cameraPosWorld.xy);

  vec2 cameraPos = vec2(0, cameraPosWorld.z);
 
  vec2 objectPos = vec2(horizontalDist, objectPosWorld.z);

  vec2 viewDir = -normalize(cameraPos - objectPos);

  return getMaxAtmosphereThickness(objectPos, viewDir);
}

vec2 getMaxAtmosphereThicknessFromCamera(vec3 objectPosWorld)
{
  float horizontalDist = distance(objectPosWorld.xy, cameraPosWorld.xy);

  vec2 cameraPos = vec2(0, cameraPosWorld.z);
 
  vec2 objectPos = vec2(horizontalDist, objectPosWorld.z);
  
  vec2 viewDir = normalize(objectPos - cameraPos);
  
  
  return getMaxAtmosphereThickness(cameraPos, viewDir);
}

vec2 getMaxAtmosphereThicknessFromObjectReverse(vec3 objectPosWorld)
{
  float horizontalDist = distance(objectPosWorld.xy, cameraPosWorld.xy);

  vec2 cameraPos = vec2(0, cameraPosWorld.z);
  vec2 objectPos = vec2(horizontalDist, objectPosWorld.z);

  vec2 viewDir = normalize(cameraPos - objectPos);

  return getMaxAtmosphereThickness(objectPos, viewDir);
}

vec2 getMaxAtmosphereThicknessFromCameraReverse(vec3 objectPosWorld)
{
  float horizontalDist = distance(objectPosWorld.xy, cameraPosWorld.xy);

  vec2 cameraPos = vec2(0, cameraPosWorld.z);
  vec2 objectPos = vec2(horizontalDist, objectPosWorld.z);
  objectPos.x *= -1;
  
  vec2 viewDir = -normalize(objectPos - cameraPos);

  return getMaxAtmosphereThickness(cameraPos, viewDir);
}

float calcOpacity(float dist)
{
  return 1.0 - exp(-3.0 * dist);
  
//   return 1.0 - exp(-pow(3.0 * dist, 2));
}


float hazeForDistance(float dist)
{
  return  1.0 - exp(-3.0 * (dist / HAZE_VISIBILITY));
}


const vec3 rgb_wavelengths = vec3(0.68, 0.55, 0.44);

const vec3 rayleigh_rgb_proportion =
  vec3((1.0/pow(rgb_wavelengths.r,4)) / (1.0/pow(rgb_wavelengths.b,4)),
       (1.0/pow(rgb_wavelengths.g,4)) / (1.0/pow(rgb_wavelengths.b,4)),
       1.0);

vec4 calcAtmosphereColor(float air_dist, float haze_dist, vec3 viewDir,
    out vec3 fog_color, out vec3 mie_color)
{
  float d = air_dist / (atmosphereVisibility);
//   float v = 25 * 1000 * 1000;
//   float d = dist / v;

  float brightness = smoothstep(0.0, 0.25, sunDir.z);

  vec3 rayleighColor = rayleigh_rgb_proportion * 0.9;
  vec3 rayleighColorLow = rayleighColor * vec3(1.0, 0.6, 0.5);
  rayleighColor = mix(rayleighColorLow, rayleighColor, brightness);

  vec3 diffuseScatteringColorDark = vec3(0.2, 0.62, 1.0);

  vec3 diffuseScatteringColorDarkLow = diffuseScatteringColorDark * vec3(1.0, 0.5, 0.3);
  diffuseScatteringColorDark = mix(diffuseScatteringColorDarkLow, diffuseScatteringColorDark, brightness);

  rayleighColor = mix(rayleighColor, vec3(1), hazyness);
  diffuseScatteringColorDark = mix(diffuseScatteringColorDark, vec3(1), hazyness);

//   vec3 diffuseScatteringColorBright = vec3(0.7, 0.9, 1.0);
  vec3 diffuseScatteringColorBright = vec3(0.4, 0.8, 1.0);
  diffuseScatteringColorBright = mix(diffuseScatteringColorBright, vec3(1), 0.5);

  fog_color = vec3(0.8, 0.93, 1.0);
  vec3 fog_color_low = fog_color * 0.6 * vec3(1.0, 0.83, 0.75);
  fog_color = mix(fog_color_low, fog_color, brightness);

  vec3 fog_color_low_alt = vec3(0.76, 0.87, 0.98);
  vec3 fog_color_low_alt_low = fog_color_low_alt * 0.6 * vec3(0.8, 0.7, 0.64);
  fog_color_low_alt = mix(fog_color_low_alt_low, fog_color_low_alt, brightness);
  fog_color = mix(fog_color_low_alt, fog_color, smoothstep(0, 5000, cameraPosWorld.z));

  vec3 diffuseScatteringColorBrightLow = diffuseScatteringColorBright * vec3(0.79, 0.57, 0.5);
  diffuseScatteringColorBright = mix(diffuseScatteringColorBrightLow,
    diffuseScatteringColorBright, brightness);

  float diffuseScatteringAmount = 1.0;
  
//   float opacity = 1.0 - exp(-3 * d * 1.0);
  float opacity = calcOpacity(d * 1.5);
  
//   rayleighColor *= 1.0 - exp(-3.0 * d  * 15.0);
//   rayleighColor = mix(rayleighColor, diffuseScatteringColorDark, 1.0 - exp(-3 * d *  4.0));
//   rayleighColor = mix(rayleighColor, diffuseScatteringColorBright, 1.0 - exp(-3 * d * 2.0));
  rayleighColor *= calcOpacity(15 * d);
  rayleighColor = mix(rayleighColor, diffuseScatteringColorDark, calcOpacity(4 * d));
  rayleighColor = mix(rayleighColor, diffuseScatteringColorBright, calcOpacity(2 * d));
  rayleighColor *= smoothstep(-0.5, 0.0, sunDir.z);

  float mie_phase = miePhase(viewDir, sunDir, MIE_PHASE_COEFFICIENT);

  vec3 mieColor = mix(vec3(1.0, 0.9, 0.5), vec3(1), smoothstep(0.0, 0.25, sunDir.z));
  mieColor = mix(mieColor * vec3(1.0, 0.6, 0.1), mieColor, smoothstep(0.0, 0.2, sunDir.z));
  mieColor = mix(mieColor * vec3(1.0, 0.7, 0.0), mieColor, mie_phase*mie_phase *
      (smoothstep(-0.1, 0.0, sunDir.z)));
  mieColor = mix(mieColor * vec3(1.0, 0.6, 0.0), mieColor, smoothstep(-0.5, -0.1, sunDir.z));

  const float MIE_SCALE = 1.3;

  fog_color *= smoothstep(-0.5, 0.0, sunDir.z);
  mieColor *= mie_phase;
  mieColor *= MIE_SCALE;
  mieColor *= hazeForDistance(haze_dist);


  fog_color = mix(fog_color, vec3(1), smoothstep(-0.5, 0.4, sunDir.z) * mieColor);

  mie_color = mieColor;

  fog_color = applyColorCorrection(fog_color);
  rayleighColor = applyColorCorrection(rayleighColor);

  return vec4(rayleighColor, opacity);
}


vec3 intersect(vec3 lineP,
               vec3 lineN,
               vec3 planeN,
               float  planeD)
{
    float distance = (planeD - dot(planeN, lineP)) / dot(lineN, planeN);
    return lineP + lineN * distance;
}


float calcHazeTransitionDistance(float baseHeight, float layerTop, vec3 cameraPos, vec3 worldPos)
{
    vec3 minPos = cameraPos.z < worldPos.z ? cameraPos : worldPos;
    vec3 maxPos = cameraPos.z < worldPos.z ? worldPos : cameraPos;

    // only the distance below the fog layer top is relevant
    maxPos = (maxPos.z > layerTop && minPos.z != maxPos.z) ?
            intersect(minPos, maxPos - minPos, vec3(0, 0, 1), layerTop) :
            maxPos;
    minPos = minPos.z > layerTop ? maxPos : minPos;

    minPos = (minPos.z < baseHeight && minPos.z != maxPos.z) ?
        intersect(minPos, maxPos - minPos, vec3(0, 0, 1), baseHeight) :
        minPos;

    maxPos = maxPos.z < baseHeight ? minPos : maxPos;

    float minHeight = minPos.z;
    float maxHeight = maxPos.z;

    minHeight = clamp(minHeight, baseHeight, layerTop);
    maxHeight = clamp(maxHeight, baseHeight, layerTop);

    float deltaHeight = maxHeight - minHeight;

    float h0 = clamp((minHeight - baseHeight) / (layerTop - baseHeight), 0, 1);
    float h1 = clamp((maxHeight - baseHeight) / (layerTop - baseHeight), 0, 1);
    float delta = h1 - h0;

#define DENSITY_ANTIDERIVATIVE(h) (pow(h,4) / 2 - pow(h,3) + h)

    float avgDensityScale = deltaHeight != 0.f ?
                (DENSITY_ANTIDERIVATIVE(h1) - DENSITY_ANTIDERIVATIVE(h0))
            / delta
            :
        0;

#undef DENSITY_ANTIDERIVATIVE

    avgDensityScale = abs(avgDensityScale);

    return length(minPos - maxPos) * avgDensityScale;
}


float calcHazeDistanceConstantDenisty(float baseHeight, float layerTop, vec3 cameraPos, vec3 worldPos)
{
    vec3 minPos = cameraPos.z < worldPos.z ? cameraPos : worldPos;
    vec3 maxPos = cameraPos.z < worldPos.z ? worldPos : cameraPos;

    // only the distance below the fog layer top is relevant
    maxPos = (maxPos.z > layerTop && minPos.z != maxPos.z) ?
            intersect(minPos, maxPos - minPos, vec3(0, 0, 1), layerTop) :
            maxPos;
    minPos = minPos.z > layerTop ? maxPos : minPos;

    minPos = (minPos.z < baseHeight && minPos.z != maxPos.z) ?
        intersect(minPos, maxPos - minPos, vec3(0, 0, 1), baseHeight) :
        minPos;

    maxPos = maxPos.z < baseHeight ? minPos : maxPos;

    return length(minPos - maxPos);
}


float calcHazeDistance(vec3 obj_pos, vec3 obj_pos_flat)
{
  vec3 viewDir = normalize(obj_pos - cameraPosWorld);
  float obj_dist = distance(obj_pos, cameraPosWorld);

  float sphere_radius = planet_radius + GROUND_FOG_HEIGHT + 100;
  vec3 sphere_center = vec3(cameraPosWorld.xy, -planet_radius);

  vec3 view_dir_view = normalize((world2ViewMatrix * vec4(viewDir, 0)).xyz);

  float spherical_fog_dist = sphericalFogDistance(cameraPosWorld, viewDir, obj_dist, sphere_center, sphere_radius);

  float fog_distance = 0;
  fog_distance += calcHazeDistanceConstantDenisty(0, GROUND_FOG_HEIGHT, cameraPosWorld, obj_pos_flat);
  fog_distance += calcHazeTransitionDistance(GROUND_FOG_HEIGHT, GROUND_FOG_HEIGHT + 200, cameraPosWorld, obj_pos_flat);

  fog_distance = mix(fog_distance, spherical_fog_dist, smoothstep(3000, 6000, cameraPosWorld.z));

  fog_distance *= GROUND_FOG_DENSITY_SCALE;

  return fog_distance;
}

void apply_fog()
{

#if !ENABLE_FOG
  return;
#endif

  debugColor = vec3(0);

  vec3 viewDir = normalize(passObjectPos - cameraPosWorld);

  vec2 t = vec2(0.0);

  if
    (
        viewDir.z < 0.0
//       gl_FragCoord.x > 800 ||
//       intersectsGround(vec3(0, 0, cameraPosWorld.z + planet_radius), viewDir)
    )
  {
    if (cameraPosWorld.z > atmosphereHeight)
    {
      t = getMaxAtmosphereThicknessFromObjectReverse(passObjectPos);
    }
    else
    {
      vec2 t1 = getMaxAtmosphereThicknessFromObjectReverse(passObjectPos);
      vec2 t2 = getMaxAtmosphereThicknessFromCameraReverse(passObjectPos);
      t = t1 - t2;

//       t = t2;
      if (t1.x < t2.x) {
        debugColor = vec3(1,1,0);
      }
    }

  }
  else {
    vec2 t1 = getMaxAtmosphereThicknessFromCamera(passObjectPos);
    vec2 t2 = getMaxAtmosphereThicknessFromObject(passObjectPos);
    t = t1 - t2;
  }

  t = max(vec2(0), t);

  vec3 fog_color;

  float fog_dist = 0;
  fog_dist += calcHazeDistance(passObjectPos, passObjectPosFlat);
  fog_dist += t.y;

  vec3 mie_color = vec3(0);
  vec4 atmosphereColor = calcAtmosphereColor(t.x, fog_dist, viewDir, fog_color, mie_color);

  float fog = hazeForDistance(fog_dist);

  float extinction = atmosphereColor.w;

  gl_FragColor.xyz *= 1.0 - extinction;

//   atmosphereColor *=  mix(0.75, 1.0, 1.0 - exp(-3 * (t/atmosphereVisibility) * 5.0));
//   else if (gl_FragCoord.x > 1200)
//     atmosphereColor *= 1.0 - exp(-3 * (t/atmosphereVisibility) * 10.0);
  
//   atmosphereColor.xyz *= atmosphereColor.w;
//   pow(atmosphereColor.w, 1.5);

//   atmosphereColor *= 0;

  gl_FragColor.xyz = mix(gl_FragColor.xyz, vec3(1), atmosphereColor.xyz);

  gl_FragColor.xyz = mix(gl_FragColor.xyz, fog_color, fog);

  gl_FragColor.xyz = applyColorCorrection(gl_FragColor.xyz);

#if 0
  if (t == -1.0) {
    gl_FragColor.xyz = vec3(0.5, 0.5, 0.0);
  }
  else if (t.x < 0.0) {
    gl_FragColor.xyz = vec3(1,0,1);
//     gl_FragColor.xyz = vec3(0.0, abs(t + 1.0) * 4.0, 0.0);
  }

  if (debugColor != vec3(0)) {
    gl_FragColor.xyz = debugColor;
  }
#endif
}
