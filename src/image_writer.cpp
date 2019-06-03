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

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

bool render_util::saveImage(const std::string &file_path,
                 int num_channels,
                 int width,
                 int height,
                 const unsigned char* image_data,
                 size_t image_data_size,
                 ImageType type)
{
  assert(image_data_size == width * height * num_channels);

  int res = 0;

  switch (type)
  {
    case ImageType::TGA:
      res = stbi_write_tga(file_path.c_str(), width, height, num_channels, image_data);
      break;
    case ImageType::PNG:
      res = stbi_write_png(file_path.c_str(), width, height, num_channels, image_data, 0);
      break;
    default:
      assert(0);
      abort();
  }

  return res != 0;
}
