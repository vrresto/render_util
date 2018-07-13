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

#ifndef ENGINE_IMAGE_LOADER_H
#define ENGINE_IMAGE_LOADER_H

#include <string>
#include <vector>
#include <cassert>
#include <SOIL/SOIL.h>

namespace render_util
{
  template <typename T>
  T *loadImageFromMemory(const std::vector<char> &data)
  {
    int width, height, channels;
    unsigned char *image_data =  
        SOIL_load_image_from_memory(
          (const unsigned char*)data.data(),
          data.size(),
          &width, &height, &channels, T::BYTES_PER_PIXEL);

    assert(image_data);

    const int size = width * height * T::BYTES_PER_PIXEL;

    T *image = new T(width, height, size, image_data);

    SOIL_free_image_data(image_data);

    return image;
  }
  
  template <typename T>
  T *loadImageFromFile(const std::string &file_path)
  {
    int width, height, channels;
    unsigned char *image_data =  
        SOIL_load_image(file_path.c_str(), &width, &height, &channels, T::BYTES_PER_PIXEL);

    if (!image_data)
      return 0;

    const int size = width * height * T::BYTES_PER_PIXEL;

    T *image = new T(width, height, size, image_data);

    SOIL_free_image_data(image_data);

    return image;
  }

  template <typename T>
  void saveImageToFile(const std::string &file_path, const T *image)
  {
    int res = SOIL_save_image(file_path.c_str(),
                    SOIL_SAVE_TYPE_TGA, image->w(), image->h(), T::BYTES_PER_PIXEL,
                    image->data());
    assert(res);
//     assert(0);
  }


}

#endif
