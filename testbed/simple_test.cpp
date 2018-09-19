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
#include <render_util/image_loader.h>
#include <util.h>
#include <FastNoise.h>

#include <gl_wrapper/gl_functions.h>

using namespace std;
using namespace glm;
using namespace render_util;


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


int main(int argc, char **argv)
{
  Image<float>::Ptr heightmap;

  if (argc == 2)
  {
    auto hm = loadImageFromFile<ImageGreyScale>(argv[1]);
    assert(hm);
    auto hm_float = heightmap = image::convert<float>(hm);

    heightmap = render_util::image::create<float>(0, ivec2(2048));

    image::blit(hm_float.get(), heightmap.get(), glm::ivec2(0));

    heightmap->forEach( [] (float &pixel) { pixel *= 10; } );
  }
  else
  {
    FastNoise noise_generator;

    heightmap = render_util::image::create<float>(0, ivec2(2048));

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
  }

  render_util::viewer::runHeightMapViewer(heightmap);
}
