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

#include <render_util/gl_binding/gl_functions.h>

using namespace render_util;
using namespace std;


namespace render_util
{


std::shared_ptr<TerrainBase> createTerrain(TextureManager &tex_mgr, bool use_lod,
                                           const string &shader_path)
{
  assert(use_lod);
  auto terrain = g_terrain_cdlod_factory(tex_mgr, shader_path);

//   auto terrain = use_lod ?
//     g_terrain_cdlod_factory() :
//     g_terrain_factory();

  return terrain;
}


} // namespace terrain_util
