#ifndef RENDER_UTIL_TERRAIN_SUBSYSTEM_H
#define RENDER_UTIL_TERRAIN_SUBSYSTEM_H

#include "layer.h"
#include <render_util/terrain_base.h>

#include <vector>

namespace render_util::terrain
{


class Subsystem
{
public:
  virtual const ShaderParameters &getShaderParameters() const = 0;
  virtual void bindTextures(TextureManager&) = 0;
  virtual void unbindTextures(TextureManager&) = 0;
  virtual void setUniforms(ShaderProgramPtr program) const {}
  virtual void updateAnimation(float frame_delta) {}

  virtual void loadLayer(Layer&, const TerrainBase::Loader::Layer&,
                         bool is_base_layer/*FIXME HACK*/) const = 0;

};


}

#endif
