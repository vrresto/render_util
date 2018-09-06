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

#ifndef RENDER_UTIL_IMAGE_LOADER_H
#define RENDER_UTIL_IMAGE_LOADER_H

#include <util.h>

#include <string>
#include <vector>
#include <memory>
#include <cassert>

namespace render_util
{


  bool loadImageFromMemory(const std::vector<char> &data_in,
                           int num_channels,
                           std::vector<unsigned char> &data_out,
                           int &width,
                           int &height);

  bool saveImage(const std::string &file_path,
                 int num_channels,
                 int width,
                 int height,
                 const unsigned char* image_data,
                 size_t image_data_size);

  template <typename T>
  std::shared_ptr<T> loadImageFromMemory(const std::vector<char> &data)
  {
    static_assert(sizeof(typename T::ComponentType) == sizeof(unsigned char));

    std::vector<unsigned char> image_data;
    int width = 0, height = 0;
    if (loadImageFromMemory(data, T::BYTES_PER_PIXEL, image_data, width, height))
    {
      return std::make_shared<T>(width, height, std::move(image_data));
    }
    else
    {
      return {};
    }
  }

  template <typename T>
  std::shared_ptr<T> loadImageFromFile(const std::string &file_path)
  {
    std::vector<char> data;
    if (util::readFile(file_path, data))
      return loadImageFromMemory<T>(data);
    else
      return {};
  }

  template <typename T>
  void saveImageToFile(const std::string &file_path, const T *image)
  {
    saveImage(file_path,
              T::BYTES_PER_PIXEL,
              image->w(),
              image->h(),
              image->getData(),
              image->dataSize());
  }


}

#endif
