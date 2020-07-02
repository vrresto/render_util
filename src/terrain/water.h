#ifndef RENDER_UTIL_TERRAIN_WATER_H
#define RENDER_UTIL_TERRAIN_WATER_H

#include "subsystem.h"
#include "layer.h"
#include <render_util/terrain_base.h>

namespace render_util::terrain
{


class WaterAnimation;


class Water : public Subsystem
{
  const TextureManager &m_texture_manager;
  ShaderParameters m_shader_params;
  TexturePtr m_animation_normal_maps;
  TexturePtr m_animation_foam_masks;
  glm::vec3 m_water_color = glm::vec3(0);
  std::unique_ptr<WaterAnimation> m_animation;

public:

  Water(const TextureManager &texture_manager, TerrainBase::BuildParameters&);
  ~Water();

  const ShaderParameters &getShaderParameters() const override { return m_shader_params; }

  void bindTextures(TextureManager&) override;
  void unbindTextures(TextureManager&) override;
  void setUniforms(ShaderProgramPtr program) const override;
  void updateAnimation(float frame_delta) override;

  void loadLayer(Layer&, const TerrainBase::Loader::Layer&,
                 bool is_base_layer/*FIXME HACK*/) const override;
};


}

#endif
