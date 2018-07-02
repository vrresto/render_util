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

#include <engine/image.h>

using namespace std;

namespace engine::image
{
    RGBA getAverageColor(const ImageRGBA *image)
    {
      RGBA average_color;
      
      float num_pixels = image->w() * image->h();

      int r = 0, g = 0, b = 0, a = 0;
      
      for (int y = 0; y < image->h(); y++) {
        for (int x = 0; x < image->w(); x++) {
          RGBA p = image->getPixel(x, y);
          a += p[4];
        }
      }
      a /= num_pixels;
//       cout<<"a:"<<a<<endl;
//       exit(1);
      
      num_pixels = 0;

      for (int y = 0; y < image->h(); y++) {
        for (int x = 0; x < image->w(); x++) {
          RGBA p = image->getPixel(x, y);
          
          if (p[4] > 10)
          {

  //           float relative_a = (float)p.a / a;
  //           cout<<"relative_a: "<<relative_a<<endl;
            
            float relative_a = (float)p[4] / 255;
            

            r += (int)p[0] * relative_a;
            g += (int)p[1] * relative_a;
            b += (int)p[2] * relative_a;
//             a += (int)p.a * relative_a;
            
            num_pixels += relative_a;
          }

        }
      }

      assert(num_pixels);
      average_color[0] = r / num_pixels;
      average_color[1] = g / num_pixels;
      average_color[2] = b / num_pixels;
      average_color[3] = a;
      
      return average_color;
    }
}
