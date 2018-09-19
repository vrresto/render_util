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
                                      const string &shader_program_name)
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

  terrain_program = createShaderProgram(name, tex_mgr, shader_path, attribute_locations);

  CHECK_GL_ERROR();

  return terrain_program;
}


} // namespace

namespace render_util
{


TerrainRenderer::TerrainRenderer(std::shared_ptr<TerrainBase> terrain, ShaderProgramPtr program) :
  m_terrain(terrain),
  m_program(program)
{
}


TerrainRenderer createTerrainRenderer(TextureManager &tex_mgr, bool use_lod, const string &shader_path,
                                      const string &shader_program_name)
{
  auto program = createTerrainProgram(tex_mgr, use_lod, shader_path, shader_program_name);

  auto terrain = use_lod ?
    g_terrain_cdlod_factory() :
    g_terrain_factory();
  terrain->setTextureManager(&tex_mgr);

  return TerrainRenderer(terrain, program);
}


} // namespace terrain_util
