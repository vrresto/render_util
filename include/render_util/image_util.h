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

#ifndef ENGINE_IMAGE_UTIL_H
#define ENGINE_IMAGE_UTIL_H

#include <render_util/image.h>

namespace engine::image
{


RGBA getAverageColor(const ImageRGBA *image);

template <typename T>
typename T::Ptr
clone(typename T::Ptr image)
{
  return typename T::Ptr(new T(image->w(), image->h(), image->dataSize(), image->getData()));
}


template <typename T>
typename T::Ptr
clone(typename T::ConstPtr image)
{
  return typename T::Ptr(new T(image->w(), image->h(), image->dataSize(), image->getData()));
}


template <typename T>
void
fill(typename T::Ptr image, typename T::PixelType color)
{
  for (int y = 0; y < image->h(); y++)
  {
    for (int x = 0; x < image->w(); x++)
    {
      image->setPixel(x, y, color);
    }
  }
}

template <typename T, int N>
std::shared_ptr<Image<T,N>>
create(Pixel<T,N> color, glm::ivec2 size)
{
  auto image = std::make_shared<Image<T,N>>(size);
  for (int y = 0; y < image->h(); y++)
  {
    for (int x = 0; x < image->w(); x++)
    {
      image->setPixel(x, y, color);
    }
  }
  return image;
}

template <typename T>
std::shared_ptr<Image<T,1>>
create(const T &color, glm::ivec2 size)
{
  auto image = std::make_shared<Image<T,1>>(size);
  for (int y = 0; y < image->h(); y++)
  {
    for (int x = 0; x < image->w(); x++)
    {
      image->at(x,y) = color;
    }
  }
  return image;
}


template<class T_dst, class T_src>
std::shared_ptr<Image<T_dst, T_src::NUM_COMPONENTS>>
convert(std::shared_ptr<const T_src> src)
{
  auto dst = std::make_shared<Image<T_dst, T_src::NUM_COMPONENTS>>(src->size());

  for (int y = 0; y < src->h(); y++)
  {
    for (int x = 0; x < src->w(); x++)
    {
      for (int i = 0; i < T_src::NUM_COMPONENTS; i++)
      {
        dst->at(x,y,i) = src->get(x,y,i);
      }
    }
  }

  return dst;
}

template<class T_dst, class T_src>
std::shared_ptr<Image<T_dst, T_src::NUM_COMPONENTS>>
convert(std::shared_ptr<T_src> src)
{
  return convert<T_dst, T_src>(std::shared_ptr<T_src>(src));
}

template <typename T>
typename T::Ptr
flipX(typename T::ConstPtr src)
{
  typename T::Ptr dst(new T(src->size()));

  for (int y = 0; y < src->h(); y++)
  {
    for (int x = 0; x < src->w(); x++)
    {
      int new_x = (src->w() - 1) - x;
      dst->setPixel(new_x, y, src->getPixel(x, y));
    }
  }

  return dst;
}


template <typename T>
typename T::Ptr
flipY(typename T::ConstPtr src)
{
  typename T::Ptr dst(new T(src->size()));

  for (int y = 0; y < src->h(); y++)
  {
    for (int x = 0; x < src->w(); x++)
    {
      int new_y = (src->h() - 1) - y;
      dst->setPixel(x, new_y, src->getPixel(x, y));
    }
  }

  return dst;
}

template <typename T>
typename T::Ptr
swapXY(typename T::ConstPtr src)
{
  assert(src->w() == src->h());

  typename T::Ptr dst(new T(src->size()));

  for (int y = 0; y < src->h(); y++)
  {
    for (int x = 0; x < src->w(); x++)
    {
      dst->setPixel(y, x, src->getPixel(x, y));
    }
  }

  return dst;
}


template <typename T>
typename T::Ptr
subImage(const T *src, int x_src, int y_src, int w, int h)
{
//       using namespace std;
//       cout<<y_src<<endl;
//       cout<<y_src+h<<endl;
  assert(x_src + w-1 < src->w());
  assert(y_src + h-1 < src->h());

  typename T::Ptr dst(new T(glm::ivec2(w,h)));

  for (int y = 0; y < h; y++)
  {
    for (int x = 0; x < w; x++)
    {
      dst->setPixel(x, y, src->getPixel(x_src + x, y_src + y));
    }
  }

  return dst;
}

template <typename T>
void
blit(const T *src, T *dst, glm::ivec2 pos)
{
//       if (pos.x >= dst->w())
//       {
//         std::cout<<"pos.x:"<<pos.x<<std::endl;
//         std::cout<<"dst->w():"<<dst->w()<<std::endl;
//       }
  
  assert(pos.x < dst->w());
  assert(pos.y < dst->h());
  
  assert(pos.x + src->w() <= dst->w());
  assert(pos.y + src->h() <= dst->h());

  for (int y = 0; y < src->h(); y++)
  {
    for (int x = 0; x < src->w(); x++)
    {
      int dst_x = pos.x + x;
      int dst_y = pos.y + y;
      dst->setPixel(dst_x, dst_y, src->getPixel(x, y));
    }
  }
}

template <typename T>
typename T::Ptr
merge(const Array2D<typename T::ConstPtr> &chunks, const glm::ivec2 &chunk_size)
{
  auto image = std::make_shared<T>(glm::ivec2(chunk_size.x * chunks.w(), chunk_size.y * chunks.h()));

  for (unsigned y = 0; y < chunks.h(); y++)
  {
    for (unsigned x = 0; x < chunks.w(); x++)
    {
      using namespace std;
      
      assert(chunks.at(x,y)->size() == chunk_size);

      glm::ivec2 pos;
      pos.x = x * chunk_size.x;
      pos.y = image->h() - ((y + 1) * chunk_size.y);

      cout<<"pos: "<<pos.x<<","<<pos.y<<" image: "<<image->size().x<<","<<image->size().y<<endl;

//           assert((pos + chunk_size).x <= image->size().x);
//           assert((pos + chunk_size).y <= image->size().y);

      blit<T>(chunks.at(x,y).get(), image.get(), pos);
    }
  }

  return image;
}


} // namespace engine::image

#endif
