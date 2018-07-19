
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

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool render_util::loadImageFromMemory(const std::vector<char> &data_in,
                          int num_channels,
                          std::vector<unsigned char> &data_out,
                          int &width,
                          int &height)
{
  int actual_channels = 0;

  unsigned char *image_data =
    stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data_in.data()),
                          data_in.size(),
                          &width,
                          &height,
                          &actual_channels,
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
    std::cout << "error loading image: " << stbi_failure_reason() << std::endl;
    return false;
  }
}
