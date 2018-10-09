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

#ifndef RENDER_UTIL_TEXTURE_MANAGER_H
#define RENDER_UTIL_TEXTURE_MANAGER_H

#include <memory>
#include <map>
#include <glm/glm.hpp>

namespace render_util
{


class Texture;

typedef std::shared_ptr<Texture> TexturePtr;


class TemporaryTextureBinding
{
  TexturePtr m_texture;
  unsigned int m_previous_binding = 0;

public:
  TemporaryTextureBinding(TexturePtr texture);
  ~TemporaryTextureBinding();
};


class Texture
{
  unsigned int m_id = 0;
  unsigned int m_target = 0;

  Texture() {}

public:
  ~Texture();

  unsigned int getID() { return m_id; }
  unsigned int getTarget() { return m_target; }
  void bind();

  static std::shared_ptr<Texture> create(unsigned int target);
};


void applyTextureParameter(unsigned int name, int value, unsigned int target);
void applyTextureParameter(unsigned int name, unsigned int value, unsigned int target);
void applyTextureParameter(unsigned int name, float value, unsigned int target);
void applyTextureParameter(unsigned int name, const glm::vec4 &value, unsigned int target);


template <typename T>
class TextureParameters
{
  std::map<unsigned int, T> m_values;


public:
  void set(unsigned int name, T value)
  {
    m_values[name] = value;
  }

  void apply(TexturePtr texture)
  {
    TemporaryTextureBinding tmp(texture);
    for (auto it : m_values)
    {
      applyTextureParameter(it.first, it.second, texture->getTarget());
    }
  }
};


class TextureManager
{
  struct Private;
  Private *p = 0;

public:
  TextureManager(unsigned int lowest_unit, unsigned int highest_unit = 0);
  ~TextureManager();

  void setActive(bool);
  void bind(unsigned int unit, TexturePtr texture);
  int getTexUnitNum(unsigned int unit) const;
  int getLowestUnit() const;
  int getHighestUnit() const;
};


} // namespace render_util

#endif
