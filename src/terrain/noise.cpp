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

#include "noise.h"
#include <util.h>
#include <log.h>
#include <render_util/gl_binding/gl_functions.h>

#include <FastNoise.h>


using namespace std;


namespace render_util::terrain
{


TexturePtr createNoiseTexture()
{
  LOG_INFO << "Creating noise texture ..." << endl;
  auto image = image::create<unsigned char>(0, glm::ivec2(2048));

  const double pi = util::PI;

  const double x1 = 0;
  const double y1 = 0;
  const double x2 = 800;
  const double y2 = 800;

//   siv::PerlinNoise noise_generator;
  FastNoise noise_generator;
  noise_generator.SetFrequency(0.4);

  for (int y = 0; y < image->h(); y++)
  {
    for (int x = 0; x < image->w(); x++)
    {
      double s = (double)x / image->w();
      double t = (double)y / image->h();
      double dx=x2-x1;
      double dy=y2-y1;

      double nx=x1+cos(s*2*pi)*dx/(2*pi);
      double ny=y1+cos(t*2*pi)*dy/(2*pi);
      double nz=x1+sin(s*2*pi)*dx/(2*pi);
      double nw=y1+sin(t*2*pi)*dy/(2*pi);
      
//       glm::dvec4 params(nx, ny, nz, nw);
//       glm::vec2 params(s, t);
      
//       double value = glm::perlin(params);
      
//       double value = noise_generator.octaveNoise(nx, ny, nz, nw);
      
      double value = noise_generator.GetSimplex(nx, ny, nz, nw);
     
//       double value = glm::simplex(params);
//       value = glm::clamp(value, 0.0, 1.0);
      
//       LOG_INFO<<"value:"<<value<<endl

//       assert(value <= 1);
      
      

//       assert(value < 0.006);

      assert(!isnan(value));
      assert(value >= -1);
      assert(value <= 1);

      value += 1;
      value /= 2;

//       value = glm::clamp(value, 0.0f, 1.0f);
      
//       value *= 0.0;
      
      value *= 255;

      image->at(x,y) = value;
    }
  }
  
//   image->setPixel(0, 0, 255);
//   image->setPixel(0, 1, 255);
//   image->setPixel(1, 0, 255);
//   image->setPixel(1, 1, 255);

//   saveImageToFile("noise.tga", image.get());
  
  TexturePtr texture = createTexture(image);

  TextureParameters<int> params;
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  params.apply(texture);

  LOG_INFO << "Creating noise texture ... done." << endl;

  return texture;
}


}
