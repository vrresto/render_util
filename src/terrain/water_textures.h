#ifndef RENDER_UTIL_TERRAIN_WATER_TEXTURES_H
#define RENDER_UTIL_TERRAIN_WATER_TEXTURES_H

#include "subsystem.h"
#include "layer.h"
#include <render_util/terrain_base.h>

namespace render_util::terrain
{


class WaterTextures : public Subsystem
{
  const TextureManager &m_texture_manager;
  ShaderParameters m_shader_params;

public:

  WaterTextures(const TextureManager &texture_manager, TerrainBase::BuildParameters&);

  const ShaderParameters &getShaderParameters() const override { return m_shader_params; }

  void bindTextures(TextureManager&) override {}
  void unbindTextures(TextureManager&) override {}
  void setUniforms(ShaderProgramPtr program) const override {}

  void loadLayer(Layer&, const TerrainBase::Loader::Layer&,
                 bool is_base_layer/*FIXME HACK*/) const override;
};


}

#endif
