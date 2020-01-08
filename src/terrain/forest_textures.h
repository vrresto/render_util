/**
 *    Rendering utilities
 *    Copyright (C) 2019 Jan Lepper
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

#ifndef _RENDER_UTIL_TERRAIN_FOREST_TEXTURES_H
#define _RENDER_UTIL_TERRAIN_FOREST_TEXTURES_H

#include "textures.h"
#include "layer.h"

#include <render_util/texture_manager.h>
#include <render_util/texunits.h>
#include <render_util/shader.h>
#include <render_util/image.h>
#include <render_util/terrain_base.h>

#include <glm/glm.hpp>

namespace render_util::terrain
{


class ForestTextures : public Textures
{
  const TextureManager &m_texture_manager;
  ShaderParameters m_shader_params;
  TexturePtr m_forest_layers_texture;
  TexturePtr m_forest_far_texture;

public:

  ForestTextures(const TextureManager &texture_manager, TerrainBase::BuildParameters&);

  const ShaderParameters &getShaderParameters() const override { return m_shader_params; }
  void bind(TextureManager&) override;
  void unbind(TextureManager&) override;
  void setUniforms(ShaderProgramPtr program) const override;
  void loadLayer(Layer&, const TerrainBase::Loader::Layer&,
                 bool is_base_layer/*FIXME HACK*/) const override;
};


}

#endif
