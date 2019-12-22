
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

#include <render_util/image_loader.h>
#include <log.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_TGA
#define STBI_NO_STDIO

#include "stb_image.h"

bool render_util::loadImageFromMemory(const std::vector<char> &data_in,
                          std::vector<unsigned char> &data_out,
                          int &width,
                          int &height,
                          int &num_channels)
{
  // num_channels forces the number of channels in the returned data
  // so channels_in_file can be safely ingnored
  unsigned char *image_data =
    stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data_in.data()),
                          data_in.size(),
                          &width,
                          &height,
                          &num_channels,
                          0);

  if (image_data)
  {
    data_out.resize(width * height * num_channels);
    memcpy(data_out.data(), image_data, data_out.size());

    stbi_image_free(image_data);
    image_data = 0;

    return true;
  }
  else
  {
    LOG_ERROR << "error loading image: " << stbi_failure_reason() << std::endl;
    return false;
  }
}

bool render_util::loadImageFromMemory(const std::vector<char> &data_in,
                          int num_channels,
                          std::vector<unsigned char> &data_out,
                          int &width,
                          int &height)
{
  int channels_in_file = 0;

  // num_channels forces the number of channels in the returned data
  // so channels_in_file can be safely ingnored
  unsigned char *image_data =
    stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data_in.data()),
                          data_in.size(),
                          &width,
                          &height,
                          &channels_in_file,
                          num_channels);

  if (image_data)
  {
    data_out.resize(width * height * num_channels);
    memcpy(data_out.data(), image_data, data_out.size());

    stbi_image_free(image_data);
    image_data = 0;

    return true;
  }
  else
  {
    LOG_ERROR << "error loading image: " << stbi_failure_reason() << std::endl;
    return false;
  }
}

namespace
{
  // fill 'data' with 'size' bytes.  return number of bytes actually read
  int read(void *user,char *data,int size)
  {
    auto file = static_cast<util::File*>(user);
    return file->read(data, size);
  }

  // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
  void skip(void *user,int n)
  {
    auto file = static_cast<util::File*>(user);
    file->skip(n);
  }

  // returns nonzero if we are at end of file/data
  int eof(void *user)
  {
    auto file = static_cast<util::File*>(user);
    return file->eof();
  }

  stbi_io_callbacks g_callbacks =
  {
    read,
    skip,
    eof,
  };
}


void render_util::getImageInfo(util::File &file, glm::ivec2 &size, int &num_components)
{

  int x, y, comp;

  auto res = stbi_info_from_callbacks(&g_callbacks, &file, &x, &y, &comp);
  assert(res);

  file.rewind();

  size = glm::ivec2(x,y);
  num_components = comp;
}


bool render_util::loadImage(util::File &file,
                            std::vector<unsigned char> &data_out,
                            int &width,
                            int &height,
                            int &channels)
{
  // num_channels forces the number of channels in the returned data
  // so channels_in_file can be safely ingnored
  auto image_data = stbi_load_from_callbacks(&g_callbacks, &file, &width, &height, &channels, 0);

  if (image_data)
  {
    data_out.resize(width * height * channels);
    memcpy(data_out.data(), image_data, data_out.size());

    stbi_image_free(image_data);
    image_data = 0;

    return true;
  }
  else
  {
    LOG_ERROR << "error loading image: " << stbi_failure_reason() << std::endl;
    return false;
  }
}

std::unique_ptr<render_util::GenericImage> render_util::loadImage(util::File &file)
{
  std::vector<unsigned char> data;
  int width = 0;
  int height = 0;
  int channels = 0;

  if (loadImage(file, data, width, height, channels))
  {
    return std::make_unique<GenericImage>(glm::ivec2(width, height), std::move(data), channels);
  }
  else
    return {};
}
