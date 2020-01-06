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


template <typename T, unsigned N>
struct Pixel
{
  enum { NUM_COMPONENTS = N };

  T components[N];

  T &operator[] (size_t index) { return components[index]; }
  const T &operator[] (size_t index) const { return components[index]; }
};

template <typename T>
struct Pixel<T, 1>
{
  enum { NUM_COMPONENTS = 1 };

  T components[NUM_COMPONENTS];

  Pixel(T value)
  {
    components[0] = value;
  }

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


template <typename T, unsigned N>
struct ImageComponentsConst
{
  using ComponentType = T;

  static constexpr int numComponents() { return N; }
};


template<typename T>
struct ImageComponentsVarying
{
  using ComponentType = T;

  ImageComponentsVarying(int num_components) : m_num_components(num_components) {}

  int getNumComponents() const { return m_num_components; }
  int numComponents() const { return getNumComponents(); }

private:
  int m_num_components = 0;
};


template <class T>
class ImageBase : public T
{
  using Base = T;

public:
  using typename Base::ComponentType;
  static constexpr int BYTES_PER_COMPONENT = sizeof(ComponentType);

protected:
  int _w = 0;
  int _h = 0;
  std::vector<unsigned char> _data;

  unsigned numPixels() const {
    return static_cast<unsigned>(_w * _h);
  }
  unsigned sizeBytes() const {
    return numPixels() * BYTES_PER_COMPONENT * Base::numComponents();
  }

  unsigned getPixelOffset(int x, int y) const {
    return static_cast<unsigned>((y * _w + x) * BYTES_PER_COMPONENT * Base::numComponents());
  }

  ComponentType *getPixel(int x, int y)
  {
    auto offset = getPixelOffset(x, y);
    return reinterpret_cast<ComponentType*>(_data.data() + offset);
  }

  const ComponentType *getPixel(int x, int y) const
  {
    auto offset = getPixelOffset(x, y);
    return reinterpret_cast<const ComponentType*>(_data.data() + offset);
  }

public:
  template <typename...Args>
  ImageBase(glm::ivec2 size, Args...args) : Base(args...)
  {
    _w = size.x;
    _h = size.y;
    _data.resize(sizeBytes());
  }

  template <typename...Args>
  ImageBase(glm::ivec2 size, std::vector<unsigned char> &&data, Args...args) :
    Base(args...),
    _data(std::move(data))
  {
    _w = size.x;
    _h = size.y;
    assert(_data.size() == sizeBytes());
  }

  template <typename...Args>
  ImageBase(glm::ivec2 size, const std::vector<ComponentType> &data, Args...args) :
    Base(args...)
  {
    _w = size.x;
    _h = size.y;
    _data.resize(sizeBytes());

    assert(_data.size() == data.size() * sizeof(ComponentType));
    memcpy(reinterpret_cast<void*>(_data.data()),
           reinterpret_cast<const void*>(data.data()), _data.size());
  }

  ComponentType &at(int x, int y, size_t component = 0)
  {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < _w);
    assert(y < _h);
    assert(component < Base::numComponents());
    return getPixel(x, y) [component];
  }

  ComponentType &at(const glm::ivec2 &pos, size_t component = 0)
  {
    return at(pos.x, pos.y, component);
  }

  const ComponentType &get(int x, int y, size_t component = 0) const
  {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < _w);
    assert(y < _h);
    assert(component < Base::numComponents());
    return getPixel(x, y) [component];
  }

  const ComponentType &get(const glm::ivec2 &pos, size_t component = 0) const
  {
    return get(pos.x, pos.y, component);
  }

  template <class F>
  void forEach(F func)
  {
    for (int y = 0; y < h(); y++)
    {
      for (int x = 0; x < w(); x++)
      {
        for (int i = 0; i < Base::numComponents(); i++)
        {
          func(at(x,y,i));
        }
      }
    }
  }

  template <class F>
  void forEach(int component, F func)
  {
    assert(component < Base::numComponents());
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
  int getWidth() const {
    return w();
  }
  int getHeight() const {
    return h();
  }
  const unsigned char *getData() const { return _data.data(); }
  const unsigned char *data() const { return getData(); }
  unsigned getDataSize() const { return _data.size(); }
  unsigned dataSize() const { return _data.size(); }
  const std::vector<unsigned char> &getDataContainer() const { return _data; }

  glm::ivec2 size() const { return glm::ivec2(_w, _h); }
  glm::ivec2 getSize() const { return size(); }
};


template <typename T, unsigned N = 1>
class Image : public ImageBase<ImageComponentsConst<T,N>>
{
  static_assert(N >= 1);
  using Base = ImageBase<ImageComponentsConst<T,N>>;

public:
  using PixelType = Pixel<T,N>;
  using Ptr =std::shared_ptr<Image<T,N>>;
  using ConstPtr = std::shared_ptr<const Image<T,N>>;

  static constexpr int BYTES_PER_PIXEL = sizeof(T) * N;
  static constexpr int NUM_COMPONENTS = N;

  Image(glm::ivec2 size) : Base(size) {}
  Image(glm::ivec2 size, std::vector<unsigned char> &&data) : Base(size, std::move(data)) {}
  Image(glm::ivec2 size, const std::vector<T> &data) : Base(size, data) {}

  const PixelType &getPixel(const glm::ivec2 pos) const
  {
    return getPixel(pos.x, pos.y);
  }

  const PixelType &getPixel(int x, int y) const
  {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < Base::_w);
    assert(y < Base::_h);
    return *reinterpret_cast<const PixelType*>(Base::getPixel(x, y));
  }

  void setPixel(glm::ivec2 pos, const PixelType &color)
  {
    setPixel(pos.x, pos.y, color);
  }

  void setPixel(int x, int y, const PixelType &color)
  {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < Base::_w);
    assert(y < Base::_h);
    *reinterpret_cast<PixelType*>(Base::getPixel(x, y)) = color;
  }
};


typedef ImageBase<ImageComponentsVarying<unsigned char>> GenericImage;
typedef Image<unsigned char, 1> ImageGreyScale;
typedef Image<unsigned char, 3> ImageRGB;
typedef Image<unsigned char, 4> ImageRGBA;


} // namespace render_util

#endif
