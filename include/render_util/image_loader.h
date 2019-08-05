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
#include <render_util/image.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include <cassert>

namespace render_util
{
  enum class ImageType
  {
    TGA,
    PNG
  };


  bool loadImageFromMemory(const std::vector<char> &data_in,
                           int num_channels,
                           std::vector<unsigned char> &data_out,
                           int &width,
                           int &height);

  bool loadImageFromMemory(const std::vector<char> &data_in,
                           std::vector<unsigned char> &data_out,
                           int &width,
                           int &height,
                           int &num_channels);


  bool saveImage(const std::string &file_path,
                 int num_channels,
                 int width,
                 int height,
                 const unsigned char* image_data,
                 size_t image_data_size,
                 ImageType type = ImageType::TGA);

  template <typename T>
  std::shared_ptr<T> loadImageFromMemory(const std::vector<char> &data)
  {
    static_assert(sizeof(typename T::ComponentType) == sizeof(unsigned char));

    std::vector<unsigned char> image_data;
    int width = 0, height = 0;
    if (loadImageFromMemory(data, T::BYTES_PER_PIXEL, image_data, width, height))
    {
      return std::make_shared<T>(glm::ivec2(width, height), std::move(image_data));
    }
    else
    {
      return {};
    }
  }

  inline std::shared_ptr<GenericImage> loadGenericImageFromMemory(const std::vector<char> &data)
  {
    std::vector<unsigned char> image_data;
    int width = 0, height = 0, num_channels = 0;
    if (loadImageFromMemory(data, image_data, width, height, num_channels))
    {
      return std::make_shared<GenericImage>(glm::ivec2(width, height), std::move(image_data), num_channels);
    }
    else
    {
      return {};
    }
  }

  template <>
  inline std::shared_ptr<GenericImage> loadImageFromMemory<GenericImage>(const std::vector<char> &data)
  {
    return loadGenericImageFromMemory(data);
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
  void saveImageToFile(const std::string &file_path, const T *image, ImageType type = ImageType::TGA)
  {
    saveImage(file_path,
              image->numComponents(),
              image->w(),
              image->h(),
              image->getData(),
              image->dataSize(),
              type);
  }


}

#endif
