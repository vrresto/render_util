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

#ifndef RENDER_UTIL_IMAGE_H
#define RENDER_UTIL_IMAGE_H

#include <functional>
#include <memory>
#include <vector>
#include <cstring>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace render_util
{


template <typename T>
class Array2D
{
protected:
  std::vector<T> m_data;
  unsigned width = 0;
  unsigned height = 0;

public:
  Array2D(int width, int height) : width(width), height(height)
  {
    m_data.resize(w() * h());
  }

  int w() const {
    return width;
  }
  int h() const {
    return height;
  }

  int size() { return width * height; }
  const std::vector<T> &getData() { return m_data; }

  T &at(unsigned x, unsigned y)
  {
    assert(x < width);
    assert(y < height);
    return m_data[y * width + x];
  }

  const T &at(unsigned x, unsigned y) const
  {
    assert(x < width);
    assert(y < height);
    return m_data[y * width + x];
  }

  template <class F>
  void forEach(F func)
  {
    for (unsigned y = 0; y < height; y++)
    {
      for (unsigned x = 0; x < width; x++)
      {
        func(at(x,y));
      }
    }
  }
};


template <typename T, int N>
struct Pixel
{
  enum { NUM_COMPONENTS = N };

  T components[N];

  T &operator[] (size_t index) { return components[index]; }
  const T &operator[] (size_t index) const { return components[index]; }
};

typedef Pixel<unsigned char, 3>  RGB;
typedef Pixel<unsigned char, 4>  RGBA;

inline glm::vec4 pixelToVector(const RGBA &pixel)
{
  glm::vec4 v = glm::make_vec4(pixel.components);
  v /= 255.0;
  return v;
}

inline RGBA vectorToPixel(glm::vec4 v)
{
  v *= 255.0;
  RGBA p;
  for (size_t i = 0; i < 4; i++)
    p[i] = glm::value_ptr(v)[i];
  return p;
}


template <typename T, int N = 1>
class Image
{
public:
  static const int BYTES_PER_PIXEL = sizeof(T) * N;
  static const int NUM_COMPONENTS = N;

  typedef std::shared_ptr<Image<T,N>> Ptr;
  typedef std::shared_ptr<const Image<T,N>> ConstPtr;
  typedef Pixel<T,N> PixelType;
  typedef Image<T,N> Type;
  typedef T ComponentType;

private:
  int _w = 0;
  int _h = 0;
  std::vector<unsigned char> _data;

  unsigned numPixels() const {
    return _w * _h;
  }
  unsigned sizeBytes() const {
    return numPixels() * BYTES_PER_PIXEL;
  }

  unsigned getPixelIndex(unsigned x, unsigned y) const {
    return (y * _w + x) ;
  }

  unsigned getPixelOffset(unsigned x, unsigned y) const {
    return (y * _w + x) * BYTES_PER_PIXEL;
  }
  
  unsigned getPixelOffset(unsigned index) const {
    assert(index < numPixels());
    return index * BYTES_PER_PIXEL;
  }

public:
  Image(int width, int height, unsigned data_size, const unsigned char *data = 0) {
    _w = width;
    _h = height;
    assert(data_size == sizeBytes());
    _data.resize(sizeBytes());
    assert(_data.size() == sizeBytes());
    if (data) {
//         memcpy(_data.data(), data, _data.size());
      for (size_t i = 0; i < _data.size(); i++) {
//           std::cout<<i<<" of "<<_data.size()-1<<std::endl;
//           fflush(stdout);
        _data[i] = data[i];
      }
    }
  }

  Image(int width, int height, std::vector<unsigned char> &&data) :
    _w(width),
    _h(height),
    _data(std::move(data))
  {
    assert(_data.size() == sizeBytes());
  }

  Image(glm::ivec2 size) {
    _w = size.x;
    _h = size.y;
    _data.resize(sizeBytes());
  }

  int numComponents() { return NUM_COMPONENTS; }

  const PixelType &getPixel(const glm::ivec2 pos) const
  {
    return getPixel(pos.x, pos.y);
  }

  const PixelType &getPixel(int x, int y) const
  {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < _w);
    assert(y < _h);
    return *reinterpret_cast<const PixelType*>(_data.data() + getPixelOffset(getPixelIndex(x,y)));
  }


  void setPixel(glm::ivec2 pos, const PixelType &color)
  {
    setPixel(pos.x, pos.y, color);
  }

  void setPixel(int x, int y, const PixelType &color)
  {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < _w);
    assert(y < _h);
    *reinterpret_cast<PixelType*>(_data.data() + getPixelOffset(getPixelIndex(x,y))) = color;
  }


  PixelType &pixelAt(int x, int y)
  {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < _w);
    assert(y < _h);
    return *reinterpret_cast<PixelType*>(_data.data() + getPixelOffset(getPixelIndex(x,y)));
  }

  PixelType &pixelAt(const glm::ivec2 &pos)
  {
    return pixelAt(pos.x, pos.y);
  }

  T &at(int x, int y, size_t component = 0)
  {
    return pixelAt(x,y)[component];
  }

  T &at(const glm::ivec2 &pos, size_t component = 0)
  {
    return pixelAt(pos)[component];
  }

  const T &get(int x, int y, size_t component = 0) const
  {
    return getPixel(x,y)[component];
  }

  const T &get(const glm::ivec2 &pos, size_t component = 0) const
  {
    return getPixel(pos)[component];
  }


  template <class F>
  void forEach(F func)
  {
    for (int y = 0; y < h(); y++)
    {
      for (int x = 0; x < w(); x++)
      {
        for (int i = 0; i < NUM_COMPONENTS; i++)
        {
          func(at(x,y,i));
        }
      }
    }
  }

  template <class F>
  void forEach(int component, F func)
  {
    assert(component < NUM_COMPONENTS);
    for (int y = 0; y < h(); y++)
    {
      for (int x = 0; x < w(); x++)
      {
        func(at(x,y,component));
      }
    }
  }


  int w() const { return _w; }
  int h() const { return _h; }
  const unsigned char *getData() const { return _data.data(); }
  const unsigned char *data() const { return getData(); }
  unsigned dataSize() const { return _data.size(); }
  glm::ivec2 size() const { return glm::ivec2(_w, _h); }
};


typedef Image<unsigned char, 1> ImageGreyScale;
typedef Image<unsigned char, 3> ImageRGB;
typedef Image<unsigned char, 4> ImageRGBA;


} // namespace render_util

#endif
