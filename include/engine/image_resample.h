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

#ifndef ENGINE_IMAGE_RESAMPLE_H
#define ENGINE_IMAGE_RESAMPLE_H

#include <engine/image.h>

namespace engine
{


template <typename T>
class Sampler
{
  const T *image = 0;

  const typename T::ComponentType &getPixel(glm::ivec2 pos, int component) const
  {
    using namespace glm;

    pos -= ivec2(1);

    if (pos.x < 0)
      pos.x += image->w();
    if (pos.y < 0)
      pos.y += image->h();

    return image->get(pos % image->size(), component);
  }

public:
  Sampler(const T *image) : image(image) {}
  Sampler() {}

  void setImage(const T *image)
  {
    this->image = image;
  }

  typename T::ComponentType sample(double u, double v, int component) const
  {
    using namespace glm;

    vec2 xy(u * image->w(), v * image->h());

    int x1 = floor(xy.x);
    int x2 = x1 + 1;

    int y1 = floor(xy.y);
    int y2 = y1 + 1;
    
    float x2_w = fract(xy.x);
    float x1_w = 1.0 - x2_w;
    
    float y2_w = fract(xy.y);
    float y1_w = 1.0 - y2_w;
    
    float w11 = x1_w * y1_w;
    float w12 = x1_w * y2_w;
    float w21 = x2_w * y1_w;
    float w22 = x2_w * y2_w;

    const typename T::ComponentType &c11 = getPixel(ivec2(x1,y1), component);
    const typename T::ComponentType &c12 = getPixel(ivec2(x1,y2), component);
    const typename T::ComponentType &c21 = getPixel(ivec2(x2,y1), component);
    const typename T::ComponentType &c22 = getPixel(ivec2(x2,y2), component);

    return c11 * w11 + c12 * w12 + c21 * w21 + c22 * w22;
  }

};

template <typename T>
class Surface
{
  std::shared_ptr<const T> m_image;
  std::shared_ptr<const T> m_mipmap;
  Sampler<T> m_sampler;
  glm::vec2 m_size = glm::vec2(1);
  
  void updateMipmap()
  {
    using namespace glm;

    assert(m_image->w() == m_image->h());
    assert(m_size.x == m_size.y);

    if (m_image->w() > m_size.x)
    {
      int mipmap_size = m_image->w();

      while (m_size.x < (float)mipmap_size && mipmap_size % 2  == 0)
      {
        int new_mipmap_size = mipmap_size / 2;
        if ((float)new_mipmap_size < m_size.x)
          break;
        mipmap_size = new_mipmap_size;
      }

//       std::cout<<"mipmap_size: "<<mipmap_size<<std::endl;

      assert(mipmap_size != 0);
      assert(m_image->w() % mipmap_size == 0);

      int down_sample_factor = m_image->w() / mipmap_size;

//       std::cout<<"down_sample_factor: "<<down_sample_factor<<std::endl;

      assert(mipmap_size * down_sample_factor == m_image->w());

      m_mipmap = downSample(m_image, down_sample_factor);
    }
    else
    {
      m_mipmap = m_image;
    }
    m_sampler.setImage(m_mipmap.get());
  }


public:
  Surface(std::shared_ptr<const T> image) : m_image(image), m_size(image->size())
  {
    updateMipmap();
  }

  void setSize(const glm::vec2 &size)
  {
    m_size = size;
    updateMipmap();
  }

  std::shared_ptr<const T> getMipmap() { return m_mipmap; }

  typename T::ComponentType get(glm::ivec2 pos, int component)
  {
    using namespace glm;

    pos %= m_size;

    vec2 dst_min_coord = vec2(1) / (m_size * vec2(2));
    vec2 uv = mix(dst_min_coord,
                  vec2(1) - dst_min_coord,
                  vec2(pos) / m_size) ;

    return m_sampler.sample(uv.x, uv.y, component);
  }

  typename T::ComponentType get(int x, int y, int component)
  {
    return get(glm::ivec2(x,y), component);
  }
};


template <typename T>
std::shared_ptr<T>
downSample(std::shared_ptr<const T> src, int factor)
{
  using namespace glm;
  using namespace std;
  
  assert(factor % 2 == 0);
  if (src->size() % 2 != ivec2(0))
  {
    cout<<src->size().x<<","<<src->size().y<<endl;
    cout<<src->size().x % 2<<","<<src->size().y % 2<<endl;
  }
  assert(src->size() % 2 == ivec2(0));

  auto dst = std::make_shared<T>(src->size() / factor);

  const int sampling_area_size = factor;
  const int num_snamples = sampling_area_size * sampling_area_size;

  for (int y = 0; y < dst->h(); y++)
  {
    for (int x = 0; x < dst->w(); x++)
    {
      for (int i = 0; i < T::NUM_COMPONENTS; i++)
      {
        float sum = 0;

        for (int y_sample = 0; y_sample < sampling_area_size; y_sample++)
        {
          int y_src = y * factor + y_sample;
          
          for (int x_sample = 0; x_sample < sampling_area_size; x_sample++)
          {
            int x_src = x * factor + x_sample;

            float src_color = src->get(x_src, y_src, i);

            sum += src_color / num_snamples;
          }

        }

        dst->at(x, y, i) = sum;
      }
    }
  }

  return dst;
}

template <typename T>
std::shared_ptr<T>
downSample(std::shared_ptr<T> src, int factor)
{
  return downSample(std::shared_ptr<const T>(src), factor);
}


template <typename T>
std::shared_ptr<T>
upSample(std::shared_ptr<const T> src, int factor)
{
  using namespace glm;

  assert(factor % 2 == 0);
  assert(src->size() % 2 == ivec2(0));

  auto dst = std::make_shared<T>(src->size() * factor);

  Surface<T> surface(src);
  surface.setSize(dst->size());

  for (int y = 0; y < dst->h(); y++)
  {
    for (int x = 0; x < dst->w(); x++)
    {
      for (int i = 0; i < T::NUM_COMPONENTS; i++)
      {
        dst->at(x,y,i) = surface.get(ivec2(x,y), i);
      }
    }
  }

  return dst;
}


} // namespace engine

#endif
