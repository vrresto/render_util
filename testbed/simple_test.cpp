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

#include <viewer.h>
#include <render_util.h>
#include <render_util/image_util.h>
#include <FastNoise.h>

using namespace std;
using namespace glm;

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
  
  runHeightMapViewer(heightmap);
}
