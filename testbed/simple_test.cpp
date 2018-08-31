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

#include <render_util/viewer.h>
#include <render_util/render_util.h>
#include <render_util/image_util.h>
#include <render_util/terrain_cdlod.h>
#include <render_util/shader_util.h>
#include <util.h>
#include <FastNoise.h>

#include <gl_wrapper/gl_functions.h>

using Terrain = render_util::TerrainCDLOD;
using namespace std;
using namespace glm;

namespace
{


const string shader_path = RENDER_UTIL_SHADER_DIR;


render_util::ShaderProgramPtr createTerrainProgram(const render_util::TextureManager &tex_mgr)
{
  render_util::ShaderProgramPtr program;

  CHECK_GL_ERROR();

  map<unsigned int, string> attribute_locations = { { 4, "attrib_pos" } };

  program = render_util::createShaderProgram(
      "terrain_cdlod_simple",
      tex_mgr,
      shader_path,
      attribute_locations);

  CHECK_GL_ERROR();

  return program;
}


} // namespace


namespace render_util
{
  const std::string &getResourcePath()
  {
    static string path = ".";
    return path;
  }

  const std::string &getDataPath()
  {
    static string path = ".";
    return path;
  }
}


int main()
{
  FastNoise noise_generator;

  auto heightmap = render_util::image::create<float>(0, ivec2(2048));

  for (int y = 0; y < heightmap->w(); y++)
  {
    for (int x = 0; x < heightmap->h(); x++)
    {

      float height = noise_generator.GetValueFractal(x * 1, y * 1) * 5000;
      height += (noise_generator.GetValueFractal(x * 10, y * 10) + 10) * 300;
//       float height = 1000;
      heightmap->at(x,y) = height;
    }
  }

  auto terrain_program_factory = [] (const render_util::TextureManager &tex_mgr)
  {
    return createTerrainProgram(tex_mgr);
  };

  render_util::viewer::runHeightMapViewer(heightmap, util::makeFactory<Terrain>(), terrain_program_factory);
}
