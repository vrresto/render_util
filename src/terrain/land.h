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

#ifndef _RENDER_UTIL_TERRAIN_LAND_H
#define _RENDER_UTIL_TERRAIN_LAND_H

#include "subsystem.h"
#include "layer.h"

#include <render_util/texture_manager.h>
#include <render_util/texunits.h>
#include <render_util/shader.h>
#include <render_util/image.h>
#include <render_util/terrain_base.h>

#include <glm/glm.hpp>

namespace render_util::terrain
{


class Land : public Subsystem
{
  const TextureManager &m_texture_manager;
  ShaderParameters m_shader_params;
  bool m_enable_normal_maps = false;
  std::map<unsigned, glm::uvec3> m_mapping;


// //   glm::ivec2 m_type_map_size = glm::ivec2(0);
//   TexturePtr m_type_map_texture;
//   TexturePtr m_type_map_texture_nm;
// //   glm::ivec2 m_base_type_map_size = glm::ivec2(0);
//   TexturePtr m_base_type_map_texture;
//   TexturePtr m_base_type_map_texture_nm;
  

  std::array<TexturePtr, MAX_TERRAIN_TEXUNITS> m_textures;
  std::array<TexturePtr, MAX_TERRAIN_TEXUNITS> m_textures_nm;

public:
  static constexpr float MAX_TEXTURE_SCALE = 8;
  static constexpr int TYPE_MAP_RESOLUTION_M = TerrainBase::GRID_RESOLUTION_M;

  Land(const TextureManager &texture_manager, TerrainBase::BuildParameters&);

  const ShaderParameters &getShaderParameters() const override { return m_shader_params; }

  void bindTextures(TextureManager&) override;
  void unbindTextures(TextureManager&) override;

  void loadLayer(Layer&, const TerrainBase::Loader::Layer&,
                 bool is_base_layer/*FIXME HACK*/) const override;
};


}

#endif
