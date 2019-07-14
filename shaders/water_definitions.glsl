vec3 getWaterColorSimple(vec3 pos, vec3 viewDir, float dist);

vec4 applyWater(vec4 color,
  vec3 view_dir,
  float dist,
  float waterDepth,
  vec2 mapCoords,
  vec3 pos,
  float shallow_sea_amount,
  float river_amount,
  float bank_amount);
