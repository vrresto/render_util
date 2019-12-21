#include terrain_params.h.glsl

uniform Terrain terrain;

#if @enable_type_map@

ivec2 getTypeMapSizePx()
{
  return terrain.detail_layer.type_map.size_px;
}

ivec2 getBaseTypeMapSizePx()
{
  return terrain.base_layer.type_map.size_px;
}

sampler2D getTypeMapSampler()
{
  return terrain.detail_layer.type_map.sampler;
}

sampler2D getBaseTypeMapSampler()
{
  return terrain.base_layer.type_map.sampler;
}

#endif
