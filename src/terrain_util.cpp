/**
 *    Rendering utilities
 *    Copyright (C) 2018  Jan Lepper
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

#include <render_util/terrain_util.h>
#include <render_util/shader_util.h>
#include <render_util/terrain.h>
#include <render_util/terrain_cdlod.h>

#include <gl_wrapper/gl_functions.h>

using namespace render_util;
using namespace std;
using namespace gl_wrapper::gl_functions;

namespace
{


ShaderProgramPtr createTerrainProgram(const TextureManager &tex_mgr, bool cdlod, const string &shader_path,
                                      const string &shader_program_name,
                                      bool enable_base_map,
                                      bool enable_base_water_map,
                                      bool is_editor,
                                      bool low_detail)
{
  string name = shader_program_name;
  if (name.empty())
    name = "terrain";

  if (cdlod)
    name += "_cdlod";

  ShaderProgramPtr terrain_program;

  CHECK_GL_ERROR();

  map<unsigned int, string> attribute_locations;

  if (cdlod)
    attribute_locations = { { 4, "attrib_pos" } };


  ShaderParameters params;
  params.set("enable_base_map", enable_base_map);
  params.set("enable_base_water_map", enable_base_water_map);
  params.set("is_editor", is_editor);
  params.set("low_detail", low_detail);

  terrain_program = createShaderProgram(name, tex_mgr, shader_path, attribute_locations, params);

  CHECK_GL_ERROR();

  return terrain_program;
}


} // namespace

namespace render_util
{


TerrainRenderer::TerrainRenderer(std::shared_ptr<TerrainBase> terrain,
                                 ShaderProgramPtr program,
                                 ShaderProgramPtr low_detail_program) :
  m_terrain(terrain),
  m_program(program),
  m_low_detail_program(low_detail_program)
{
}


TerrainRenderer createTerrainRenderer(TextureManager &tex_mgr, bool use_lod, const string &shader_path,
                                      const string &shader_program_name,
                                      bool enable_base_map,
                                      bool enable_base_water_map,
                                      bool is_editor)
{
  auto program = createTerrainProgram(tex_mgr,
                                      use_lod,
                                      shader_path,
                                      shader_program_name,
                                      enable_base_map,
                                      enable_base_water_map,
                                      is_editor,
                                      false);

  auto low_detail_program = createTerrainProgram(tex_mgr,
                                                 use_lod,
                                                 shader_path,
                                                 shader_program_name,
                                                 enable_base_map,
                                                 enable_base_water_map,
                                                 is_editor,
                                                 true);

  auto terrain = use_lod ?
    g_terrain_cdlod_factory() :
    g_terrain_factory();
  terrain->setTextureManager(&tex_mgr);


  return TerrainRenderer(terrain, program, low_detail_program);
}


} // namespace terrain_util
