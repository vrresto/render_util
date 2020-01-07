#ifndef RENDER_UTIL_TERRAIN_WATER_TEXTURES_H
#define RENDER_UTIL_TERRAIN_WATER_TEXTURES_H

#include "textures.h"
#include "terrain_layer.h"
#include <render_util/terrain_base.h>

namespace render_util::terrain
{


class WaterTextures : public Textures
{
  const TextureManager &m_texture_manager;
  ShaderParameters m_shader_params;
//   TexturePtr m_forest_layers_texture;
//   TexturePtr m_forest_far_texture;
  std::unique_ptr<WaterMap> m_water_map;

public:

  WaterTextures(const TextureManager &texture_manager, TerrainBase::BuildParameters&);

  const ShaderParameters &getShaderParameters() const override { return m_shader_params; }

  void bind(TextureManager&) override {}
  void unbind(TextureManager&) override {}
  void setUniforms(ShaderProgramPtr program) const override {}

  const WaterMap &getWaterMap()
  {
    assert(m_water_map);
    return *m_water_map;
  }
};


}

#endif
