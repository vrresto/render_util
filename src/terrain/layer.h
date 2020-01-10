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

#ifndef RENDER_UTIL_TERRAIN_LAYER_H
#define RENDER_UTIL_TERRAIN_LAYER_H

#include <render_util/texture_manager.h>
#include <render_util/shader.h>

#include <glm/glm.hpp>
#include <vector>
#include <optional>

namespace render_util::terrain
{


struct Map
{
  unsigned int texunit;
  int resolution_m;
  glm::vec2 size_m;
  glm::ivec2 size_px;
  TexturePtr texture;
  std::string name;
};


struct WaterMap
{
  unsigned int texunit_table;
  unsigned int texunit;

  TexturePtr texture_table;
  TexturePtr texture;

  glm::vec2 table_size_m;
  glm::vec2 table_shift_m;
  float chunk_size_m;
  glm::vec2 chunk_scale;
  glm::vec2 chunk_shift_m;
};


struct Layer
{
  glm::vec2 origin_m;
  glm::vec2 size_m;
  std::string uniform_prefix;
  std::vector<Map> maps;
  std::optional<WaterMap> water_map;

  void bindTextures(render_util::TextureManager& tex_mgr)
  {
    for (auto& map : maps)
      tex_mgr.bind(map.texunit, map.texture);
    if (water_map)
      bindWaterMap(*water_map, tex_mgr);
  }

  void unbindTextures(render_util::TextureManager& tex_mgr)
  {
    for (auto& map : maps)
      tex_mgr.unbind(map.texunit, map.texture->getTarget());
    if (water_map)
      unbindWaterMap(*water_map, tex_mgr);
  }

  void setUniforms(ShaderProgramPtr program, const render_util::TextureManager &tex_mgr)
  {
    program->setUniform(uniform_prefix + "size_m", size_m);
    program->setUniform(uniform_prefix + "origin_m", origin_m);
    for (auto& map : maps)
      setMapUniforms(map, program, tex_mgr);
    if (water_map)
      setWaterMapUniforms(*water_map, program, tex_mgr);
  }

private:
  void bindWaterMap(WaterMap &map, render_util::TextureManager& tex_mgr)
  {
    tex_mgr.bind(map.texunit, map.texture);
    tex_mgr.bind(map.texunit_table, map.texture_table);
  }

  void unbindWaterMap(WaterMap &map, render_util::TextureManager& tex_mgr)
  {
    tex_mgr.unbind(map.texunit, map.texture->getTarget());
    tex_mgr.unbind(map.texunit_table, map.texture_table->getTarget());
  }

  void setWaterMapUniforms(WaterMap &map, ShaderProgramPtr program,
                           const render_util::TextureManager &tex_mgr)
  {
    program->setUniformi(uniform_prefix + "water_map" + ".sampler",
                         tex_mgr.getTexUnitNum(map.texunit));
    program->setUniformi(uniform_prefix + "water_map" + ".sampler_table",
                         tex_mgr.getTexUnitNum(map.texunit_table));

    program->setUniform(uniform_prefix + "water_map" + ".chunk_size_m", map.chunk_size_m);
    program->setUniform(uniform_prefix + "water_map" + ".table_size_m", map.table_size_m);
    program->setUniform(uniform_prefix + "water_map" + ".table_shift_m", map.table_shift_m);
    program->setUniform(uniform_prefix + "water_map" + ".chunk_scale", map.chunk_scale);
    program->setUniform(uniform_prefix + "water_map" + ".chunk_shift_m", map.chunk_shift_m);
  }

  void setMapUniforms(Map &map, ShaderProgramPtr program,
                      const render_util::TextureManager &tex_mgr)
  {
    program->setUniformi(uniform_prefix + map.name + ".sampler", tex_mgr.getTexUnitNum(map.texunit));
    program->setUniformi(uniform_prefix + map.name + ".resolution_m", map.resolution_m);
    program->setUniform(uniform_prefix + map.name + ".size_px", map.size_px);
    program->setUniform(uniform_prefix + map.name + ".size_m", map.size_m);
  }
};


}

#endif
