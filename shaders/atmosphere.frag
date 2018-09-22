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

float getMaxAtmosphereThickness(vec2 cameraPos, vec2 viewDir);
float getMaxAtmosphereThickness(vec3 cameraPos, vec3 viewDir);

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
varying vec3 passNormal;

// const float atmosphereVisibility = 400000.0;
// const float atmosphereVisibility = 200000.0;
const float atmosphereVisibility = 600000.0;

vec3 debugColor;

void resetDebugColor() {
  debugColor = vec3(0);
}

vec3 getDebugColor() {
  return debugColor;
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


float getMaxAtmosphereThickness(float height, float mu)
{
  float r = planet_radius + height;
  float Rg = planet_radius;
  float Rt = planet_radius + atmosphereHeight;

  float p = sqrt(r*r - Rg*Rg);
  float H = sqrt(Rt*Rt - Rg*Rg);
  
  float Ur = p / H;

//   float relativeHeight = height / atmosphereHeight;

  float t = texture2D(sampler_atmosphere_thickness_map, vec2(mu, Ur)).x;
  
  if (t < 0.0)
    debugColor = vec3(0,1,1);
  
  return t;
//   return 10000.0;
}


float getMaxAtmosphereThickness(vec2 cameraPos, vec2 viewDir)
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
        return 0.0;
      }

      if (atmosphereEdgeIntersectionDist < 0)
      {
        debugColor = vec3(1, 0.5, 0);
        return 0.0;
      }

      cameraPos += viewDir * atmosphereEdgeIntersectionDist;
    }
    else
    {
      return 0.0;
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
    return 0.0;
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

float getMaxAtmosphereThicknessFromObject(vec3 objectPosWorld)
{
  float horizontalDist = distance(objectPosWorld.xy, cameraPosWorld.xy);

  vec2 cameraPos = vec2(0, cameraPosWorld.z);
 
  vec2 objectPos = vec2(horizontalDist, objectPosWorld.z);

  vec2 viewDir = -normalize(cameraPos - objectPos);

  return getMaxAtmosphereThickness(objectPos, viewDir);
}

float getMaxAtmosphereThicknessFromCamera(vec3 objectPosWorld)
{
  float horizontalDist = distance(objectPosWorld.xy, cameraPosWorld.xy);

  vec2 cameraPos = vec2(0, cameraPosWorld.z);
 
  vec2 objectPos = vec2(horizontalDist, objectPosWorld.z);
  
  vec2 viewDir = normalize(objectPos - cameraPos);
  
  
  return getMaxAtmosphereThickness(cameraPos, viewDir);
}

float getMaxAtmosphereThicknessFromObjectReverse(vec3 objectPosWorld)
{
  float horizontalDist = distance(objectPosWorld.xy, cameraPosWorld.xy);

  vec2 cameraPos = vec2(0, cameraPosWorld.z);
  vec2 objectPos = vec2(horizontalDist, objectPosWorld.z);

  vec2 viewDir = normalize(cameraPos - objectPos);

  return getMaxAtmosphereThickness(objectPos, viewDir);
}

float getMaxAtmosphereThicknessFromCameraReverse(vec3 objectPosWorld)
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

vec4 calcAtmosphereColor(float dist, vec3 viewDir)
{
  float d = dist / (atmosphereVisibility);
//   float v = 25 * 1000 * 1000;
//   float d = dist / v;

  vec3 rayleighColor = vec3(0.0, 0.25, 0.9);
  
//   rayleighColor = mix(rayleighColor, vec3(0.9, 0.95, 1.0), 0.4);

  vec3 diffuseScatteringColorDark = vec3(0.15, 0.62, 1.0);

//   if (gl_FragCoord.x > 900)
  {
    rayleighColor = mix(rayleighColor, vec3(1.0), 0.05);
    diffuseScatteringColorDark = mix(diffuseScatteringColorDark, vec3(1), 0.05);
  }


//   vec3 diffuseScatteringColorBright = vec3(0.7, 0.9, 1.0);
  vec3 diffuseScatteringColorBright = mix(diffuseScatteringColorDark, vec3(1.0), 0.75);
//   vec3 diffuseScatteringColorBright = mix(vec3(0.15, 0.75, 1.0), vec3(0.95), 0.7);

  float diffuseScatteringAmount = 1.0;
  
//   float opacity = 1.0 - exp(-3 * d * 1.0);
  float opacity = calcOpacity(d);
  
//   rayleighColor *= 1.0 - exp(-3.0 * d  * 15.0);
//   rayleighColor = mix(rayleighColor, diffuseScatteringColorDark, 1.0 - exp(-3 * d *  4.0));
//   rayleighColor = mix(rayleighColor, diffuseScatteringColorBright, 1.0 - exp(-3 * d * 2.0));
  rayleighColor *= calcOpacity(15 * d);
  rayleighColor = mix(rayleighColor, diffuseScatteringColorDark, calcOpacity(4 * d));
  rayleighColor = mix(rayleighColor, diffuseScatteringColorBright,calcOpacity(2 * d));

  
//   opacity = 1

  rayleighColor *= smoothstep(-0.4, 0.4, sunDir.z);


  float mieBrightmess = clamp(dot(viewDir, sunDir), 0, 1);
  float mie = mieBrightmess;
  mie *= mie;
  mie *= 1 - exp(-3 * d * 3);

  vec3 mieColor = mix(vec3(1.0, 0.7, 0.0), vec3(1), mieBrightmess);

  mie *= smoothstep(-0.5, 0.4, sunDir.z);
//   mie *= 0.7;
  
  mieColor *= mie;

//   rayleighColor *= 0;
  
  rayleighColor = mix(rayleighColor, vec3(1), mieColor);

  return vec4(rayleighColor, opacity);
//   return vec4(1,0,0,1);
}


void apply_fog()
{

#if !ENABLE_FOG
  return;
#endif

  debugColor = vec3(0);

  vec3 viewDir = normalize(passObjectPos - cameraPosWorld);

  float t = 0.0;

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
      float t1 = getMaxAtmosphereThicknessFromObjectReverse(passObjectPos);
      float t2 = getMaxAtmosphereThicknessFromCameraReverse(passObjectPos);
      t = t1 - t2;

//       t = t2;
      if (t1 < t2) {
        debugColor = vec3(1,1,0);
      }
    }

  }
  else {
    float t1 = getMaxAtmosphereThicknessFromCamera(passObjectPos);
    float t2 = getMaxAtmosphereThicknessFromObject(passObjectPos);
    t = t1 - t2;
  }

  vec4 atmosphereColor = calcAtmosphereColor(t, viewDir);

  float extinction = atmosphereColor.w;

  extinction *=  1.0 - exp(-3 * (t/atmosphereVisibility) * 5.0);

  gl_FragColor.xyz *= 1.0 - extinction;

  atmosphereColor *=  mix(0.75, 1.0, 1.0 - exp(-3 * (t/atmosphereVisibility) * 5.0));
    
//   else if (gl_FragCoord.x > 1200)
//     atmosphereColor *= 1.0 - exp(-3 * (t/atmosphereVisibility) * 10.0);
  
//   atmosphereColor.xyz *= atmosphereColor.w;
//   pow(atmosphereColor.w, 1.5);

//   atmosphereColor *= 0;

  gl_FragColor.xyz = mix(gl_FragColor.xyz, vec3(1), atmosphereColor.xyz);
  
  
  if (t == -1.0) {
    gl_FragColor.xyz = vec3(0.5, 0.5, 0.0);
  }
  else if (t < 0.0) {
    gl_FragColor.xyz = vec3(1,0,1);
//     gl_FragColor.xyz = vec3(0.0, abs(t + 1.0) * 4.0, 0.0);
  }

  if (debugColor != vec3(0)) {
    gl_FragColor.xyz = debugColor;
  }
}
