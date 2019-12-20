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

#ifndef _RENDER_UTIL_TERRAIN_LAND_TEXTURES_H
#define _RENDER_UTIL_TERRAIN_LAND_TEXTURES_H

#include "terrain_layer.h"

#include <render_util/texture_manager.h>
#include <render_util/texunits.h>
#include <render_util/shader.h>
#include <render_util/image.h>
#include <render_util/terrain_base.h>

#include <glm/glm.hpp>

namespace render_util::terrain
{


class LandTextures
{
  const TextureManager &m_texture_manager;
  ShaderParameters m_shader_params;
  bool m_enable_normal_maps = false;
//   glm::ivec2 m_type_map_size = glm::ivec2(0);
  TexturePtr m_type_map_texture;
  TexturePtr m_type_map_texture_nm;
//   glm::ivec2 m_base_type_map_size = glm::ivec2(0);
  TexturePtr m_base_type_map_texture;
  TexturePtr m_base_type_map_texture_nm;
  std::array<TexturePtr, MAX_TERRAIN_TEXUNITS> m_textures;
  std::array<TexturePtr, MAX_TERRAIN_TEXUNITS> m_textures_nm;

  std::vector<terrain::TerrainTextureMap> m_texture_maps;
  std::vector<terrain::TerrainTextureMap> m_base_texture_maps;

public:
  static constexpr float MAX_TEXTURE_SCALE = 8;
  static constexpr int TYPE_MAP_RESOLUTION_M = TerrainBase::GRID_RESOLUTION_M;

  LandTextures(const TextureManager &texture_manager,
                  std::vector<ImageRGBA::Ptr> &textures,
                  std::vector<ImageRGB::Ptr> &textures_nm,
                  const std::vector<float> &texture_scale,
                  TerrainBase::TypeMap::ConstPtr type_map,
                  TerrainBase::TypeMap::ConstPtr base_type_map = {});

  const ShaderParameters &getShaderParameters() const { return m_shader_params; }

  const std::vector<terrain::TerrainTextureMap> &getTextureMaps() const { return m_texture_maps; }
  const std::vector<terrain::TerrainTextureMap> &getBaseTextureMaps() const { return m_base_texture_maps; }

  void bind(TextureManager&);
  void unbind(TextureManager&);
  void setUniforms(ShaderProgramPtr program);
};


}

#endif
