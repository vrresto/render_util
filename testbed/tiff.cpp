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

#include "tiff.h"
#include <render_util/image_util.h>

#include <iostream>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <tiffio.h>

using namespace std;

// const unsigned max_map_size = 4096;
glm::ivec2 max_map_size = glm::ivec2(2 * 8192, 8192);
// const unsigned max_map_size = 40000;

render_util::Image<float>::Ptr loadTiff(const char *filename)
{
  TIFF *tif = TIFFOpen(filename, "r");
  if (!tif)
  {
    fprintf (stderr, "Can't open %s for reading\n", filename);
    exit(1);
  }

  uint16_t spp, bpp;
  uint32_t width, height;
  TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bpp);
  TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);

//   uint16_t photo;
//   TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photo);

  unsigned linesize = TIFFScanlineSize(tif);

  cout<<"width: "<<width<<endl;
  cout<<"height: "<<height<<endl;
  cout<<"TIFFTAG_BITSPERSAMPLE: "<<bpp<<endl;
  cout<<"TIFFTAG_SAMPLESPERPIXEL: "<<spp<<endl;
  
  assert(bpp == 16);
  assert(spp == 1);
  assert(linesize >= 2 * width);

//   char *buf = 0;
//   buf = (char*)malloc(linesize * height);
//   for (unsigned i = 0; i < height; i++)
//     TIFFReadScanline(tif, &buf[i * linesize], i, 0);
  
  
  char *line_buf = 0;
  line_buf = (char*)malloc(linesize);


//   float size_mb = (float)(data.size() * sizeof(int16_t)) / 1024 / 1024;
//   cout<<"tiff data size: "<<size_mb<<" MB"<<endl;
  
//   glm::ivec2 offset(1452, 12256);
//   glm::ivec2 offset(1452, 7256);
  glm::ivec2 offset(1452, 12556);


  int image_width = min((int)width, max_map_size.x);
  int image_height = min((int)height, max_map_size.y);

  auto image = render_util::image::create<float>(0, glm::ivec2(image_width, image_height));

  cout<<"reading tiff data ..."<<endl;
  for (unsigned i = 0; i < image_height; i++)
  {
    TIFFReadScanline(tif, line_buf, i + offset.y, 0);

    int16_t row[width];
    assert(sizeof(row) <= linesize);
    memcpy(row, line_buf, sizeof(row));

    for (unsigned x = 0; x < image_width; x++)
    {
      image->at(x, i) = row[x + offset.x];
    }
  }
  cout<<"reading tiff data done."<<endl;

  TIFFClose(tif);
  free(line_buf);
 
  return image;
}
