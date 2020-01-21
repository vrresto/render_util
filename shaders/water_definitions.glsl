struct WaterParameters
{
#if !LOW_DETAIL
  float wave_strength;
  float depth;
#endif

  vec3 normal;
  vec3 env_color;
};

void getWaterParameters(vec3 pos, vec3 view_dir, float dist, out WaterParameters p, vec3 pos_flat);

// vec3 getWaterColorSimple(vec3 pos, vec3 viewDir, float dist);
// vec3 getWaterColorSimple(vec3 pos, vec3 viewDir, float dist, vec3 normal, vec3 env_color);
vec3 getWaterColorSimple(vec3 pos, vec3 viewDir, float dist, in WaterParameters params);

vec3 applyWater(in vec3 color_in,
  vec3 view_dir,
  float dist,
  vec2 mapCoords,
  vec3 pos,
  float shallow_sea_amount,
  float river_amount,
  float bank_amount,
  in WaterParameters params);

void applyWater(in vec3 lit_color_in, in vec3 unlit_color_in,
  vec3 view_dir,
  float dist,
  vec2 mapCoords,
  vec3 pos,
  float shallow_sea_amount,
  float river_amount,
  float bank_amount,
  out vec3 lit_color,
  out vec3 unlit_color,
  in WaterParameters params);

vec3 calcWaterEnvColor(vec3 pos, vec3 normal, vec3 view_dir);
vec3 getWaterNormal(vec3 pos, float dist, vec2 coords);
