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

#ifndef RENDER_UTIL_TERRAIN_TERRAIN_LAYER_H
#define RENDER_UTIL_TERRAIN_TERRAIN_LAYER_H

#include <render_util/texture_manager.h>
#include <render_util/shader.h>

#include <glm/glm.hpp>
#include <vector>

namespace render_util::terrain
{


struct TerrainTextureMap
{
  unsigned int texunit;
  int resolution_m;
  glm::vec2 size_m;
  glm::ivec2 size_px;
  TexturePtr texture;
  std::string name;
};


struct TerrainLayer
{
  glm::vec2 origin_m;
  glm::vec2 size_m;
  std::string uniform_prefix;
  std::vector<TerrainTextureMap> texture_maps;

  void bindTextures(render_util::TextureManager& tex_mgr)
  {
    for (auto& map : texture_maps)
      tex_mgr.bind(map.texunit, map.texture);
  }

  void setUniforms(ShaderProgramPtr program, render_util::TextureManager &tex_mgr)
  {
    program->setUniform(uniform_prefix + "size_m", size_m);
    program->setUniform(uniform_prefix + "origin_m", origin_m);
    for (auto& map : texture_maps)
      setTextureMapUniforms(map, program, tex_mgr);
  }

private:
  void setTextureMapUniforms(TerrainTextureMap &map, ShaderProgramPtr program,
                             render_util::TextureManager &tex_mgr)
  {
    program->setUniformi(uniform_prefix + map.name + ".sampler", tex_mgr.getTexUnitNum(map.texunit));
    program->setUniformi(uniform_prefix + map.name + ".resolution_m", map.resolution_m);
    program->setUniform(uniform_prefix + map.name + ".size_px", map.size_px);
    program->setUniform(uniform_prefix + map.name + ".size_m", map.size_m);
  }
};


}

#endif
