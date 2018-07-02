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

#ifndef ELEVATION_MAP_H
#define ELEVATION_MAP_H

#include <glm/glm.hpp>

#include <vector>
#include <cassert>

namespace engine
{
 
  class ElevationMap
  {
    glm::ivec2 size = glm::ivec2(0);
    std::vector<float> data;

  public:
    ElevationMap() {}
    ElevationMap(unsigned int width, int unsigned height, const std::vector<float> &data) :
      size(width, height),
      data(data)
    {
        assert(data.size() == size.x * size.y);
    }

    ElevationMap(Image<float>::ConstPtr image) : size(image->size())
    {
      data.resize(size.x * size.y);

      for (int y = 0; y < size.y; y++)
      {
        for (int x = 0; x < size.x; x++)
        {
          float elevation = image->get(x,y);
          data[y * size.x + x] = elevation;
        }
      }
    }


    float getElevation(int x, int y) const
    {
      bool mirror_x = ((x / size.x) % 2);
      bool mirror_y = ((y / size.y) % 2);

      x = x % size.x;
      y = y % size.y;

      if (mirror_x)
        x = (size.x-1) - x;
      if (mirror_y)
        y = (size.y-1) - y;

      return data[((size.y-1) - y) * size.x + x];
    }


    int getWidth() const { return size.x; }
    int getHeight() const { return size.y; }
    glm::ivec2 getSize() const { return size; }
    const float *getData() const { return data.data(); }
  };


}

#endif
