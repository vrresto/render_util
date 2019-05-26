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

#include <render_util/texture_manager.h>
#include <render_util/texunits.h>

#include <iostream>
#include <vector>
#include <cstdio>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include <GL/gl.h>

#include <render_util/gl_binding/gl_functions.h>

using namespace render_util::gl_binding;
using std::vector;
using std::cout;
using std::endl;


namespace render_util
{


void applyTextureParameter(unsigned int name, int value, unsigned int target)
{
  gl::TexParameteri(target, name, value);
}


void applyTextureParameter(unsigned int name, const glm::vec4 &value, unsigned int target)
{
  gl::TexParameterfv(target, name, glm::value_ptr(value));
}


Texture::~Texture()
{
  if (m_id)
    gl::DeleteTextures(1, &m_id);
}


void Texture::bind()
{
  gl::BindTexture(getTarget(), getID());
}


std::shared_ptr<Texture> Texture::create(unsigned int target)
{
  std::shared_ptr<Texture> texture(new Texture);
  texture->m_target = target;
  gl::GenTextures(1, &texture->m_id);
  return texture;
}


TemporaryTextureBinding::TemporaryTextureBinding(TexturePtr texture) : m_texture(texture)
{
  switch (texture->getTarget())
  {
    case GL_TEXTURE_1D:
      gl::GetIntegerv(GL_TEXTURE_BINDING_1D, (GLint*) &m_previous_binding);
      break;
    case GL_TEXTURE_2D:
      gl::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*) &m_previous_binding);
      break;
    case GL_TEXTURE_2D_ARRAY:
      gl::GetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, (GLint*) &m_previous_binding);
      break;
    default:
      cout<<std::hex<<texture->getTarget()<<endl;
      assert(0);
  }

  m_texture->bind();
}


TemporaryTextureBinding::~TemporaryTextureBinding()
{
  gl::BindTexture(m_texture->getTarget(), m_previous_binding);
}



namespace
{


  struct Binding
  {
    unsigned int texture = 0;
    unsigned int target = 0; 
  };


  void applyBinding(unsigned int unit, const Binding &binding, TextureManager &mgr)
  {
    if (binding.target == 0)
      return;

    CHECK_GL_ERROR();

    GLenum active_unit_save;
    gl::GetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint*>(&active_unit_save));

    gl::ActiveTexture(GL_TEXTURE0 + mgr.getTexUnitNum(unit));
    CHECK_GL_ERROR();

//     cout<<"target: "<<binding.target<<endl;
//     cout<<"texture: "<<binding.texture<<endl;
    
    gl::BindTexture(binding.target, binding.texture);
    CHECK_GL_ERROR();

    gl::ActiveTexture(active_unit_save);
    CHECK_GL_ERROR();
  }

  void applyBindings(const vector<Binding> bindings, TextureManager &mgr)
  {
    for (size_t i = 0; i < bindings.size(); i++)
    {
      applyBinding(i, bindings[i], mgr);
    }
  }


} // namespace



struct TextureManager::Private
{
  unsigned int lowest_unit = 0;
  unsigned int highest_unit = 0;
  unsigned int max_units = 0;
  bool is_active = false;
  vector<Binding> bindings;
};


TextureManager::TextureManager(unsigned int lowest_unit, unsigned int highest_unit) : p(new Private)
{
  gl::GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, (int*)&p->max_units);
  cout << "max texunits: " << p->max_units << endl;

  if (!highest_unit)
    highest_unit = getMaxUnits() - 1;

  cout << "TEXUNIT_NUM: " << TEXUNIT_NUM << endl;
  cout << "highest_unit: " << highest_unit << endl;
  assert(TEXUNIT_NUM < highest_unit);
  assert(highest_unit < getMaxUnits());

  p->lowest_unit = lowest_unit;
  p->highest_unit = highest_unit;

  p->bindings.resize(p->max_units);
}

TextureManager::~TextureManager()
{
  delete p;
}


int TextureManager::getMaxUnits() const
{
  return p->max_units;
}


int TextureManager::getLowestUnit() const
{
  return p->lowest_unit;
}


int TextureManager::getHighestUnit() const
{
  return p->highest_unit;
}


int TextureManager::getTexUnitNum(unsigned int unit) const
{
  assert(!p->highest_unit || unit <= p->highest_unit);
  assert(p->lowest_unit + unit < getMaxUnits());

  return p->lowest_unit + unit;
}


void TextureManager::setActive(bool active)
{
  assert(p->is_active != active);

  if (active)
  {
    applyBindings(p->bindings, *this);
  }

  p->is_active = active;
}


void TextureManager::bind(unsigned int unit, TexturePtr texture)
{
  assert(unit <= p->highest_unit);
  assert(unit < getMaxUnits());
  Binding &binding = p->bindings[unit];

  binding.texture = texture->getID();
  binding.target = texture->getTarget();

  if (p->is_active)
    applyBinding(unit, binding, *this);
}


} // namespace render_util
