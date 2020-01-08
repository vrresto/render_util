#ifndef RENDER_UTIL_TERRAIN_TEXTURES_H
#define RENDER_UTIL_TERRAIN_TEXTURES_H

#include "layer.h"
#include <render_util/terrain_base.h>

#include <vector>

namespace render_util::terrain
{


class Textures
{
public:
  virtual const ShaderParameters &getShaderParameters() const = 0;
  virtual void bind(TextureManager&) = 0;
  virtual void unbind(TextureManager&) = 0;
  virtual void setUniforms(ShaderProgramPtr program) const {}

  virtual void loadLayer(Layer&, const TerrainBase::Loader::Layer&,
                         bool is_base_layer/*FIXME HACK*/) const = 0;

};


}

#endif
