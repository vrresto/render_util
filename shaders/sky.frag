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

vec4 calcAtmosphereColor(float dist_air, float dist_haze, vec3 viewDir, out vec3 fog_color);
vec2 getMaxAtmosphereThickness(vec2 cameraPos, vec2 viewDir);
bool intersectsGround(vec3 rayStart, vec3 rayDir);
float getCircleIntersectionDistFromOutside(vec2 rayStart, vec2 rayDir, float radius);
void resetDebugColor();
vec3 getDebugColor();

float calcHaze(vec3 obj_pos);
float hazeForDistance(float dist);

varying vec3 passObjectPosWorld;
uniform vec3 cameraPosWorld;
uniform vec3 sunDir;
uniform mat4 view2WorldMatrix;
uniform float planet_radius;


vec3 calcLight2()
{
  float ambientLight = 0.5 * smoothstep(-0.5, 0.4, sunDir.z);
  float directLight = 0.8 * smoothstep(-0.1, 0.4, sunDir.z);
  directLight *= clamp(dot(vec3(0,0,1), sunDir), 0.0, 2.0);
  float light = clamp(directLight + ambientLight, 0, 2);

  light *= 1.2;

  return vec3(light);
}


void main(void)
{
  resetDebugColor();

  gl_FragColor.w = 1.0;
  gl_FragColor.xyz = vec3(0);

  vec3 viewDir = normalize(passObjectPosWorld - cameraPosWorld);
  vec2 viewDirHorizontal = normalize(viewDir.xy);
  vec2 viewDirVertical = vec2(dot(vec3(viewDirHorizontal, 0), viewDir), viewDir.z);

  vec2 cameraPosVertical = vec2(0, cameraPosWorld.z);

  vec2 t = vec2(0);

  {
    t = getMaxAtmosphereThickness(cameraPosVertical, viewDirVertical);
  }


  vec3 fog_color;
  vec4 atmosphere_color = calcAtmosphereColor(t.x, t.y, viewDir, fog_color);
 
  vec3 obj_pos = cameraPosWorld + viewDir * 9000000;
  float fog = hazeForDistance(t.y);
  atmosphere_color.xyz = mix(atmosphere_color.xyz, fog_color, fog);

  gl_FragColor.xyz *= 1.0 - atmosphere_color.w;
  gl_FragColor.xyz = mix(gl_FragColor.xyz, vec3(1), atmosphere_color.xyz);


  float sunDisc = smoothstep(0.9999, 0.99995, dot(viewDir, sunDir));
  gl_FragColor.xyz += vec3(sunDisc);


  if (getDebugColor() != vec3(0)) {
    gl_FragColor.xyz = getDebugColor();
  }
}
