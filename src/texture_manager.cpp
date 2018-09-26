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

#include <iostream>
#include <array>
#include <cstdio>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include <GL/gl.h>

#include <gl_wrapper/gl_functions.h>

using namespace gl_wrapper::gl_functions;
using std::array;
using std::cout;
using std::endl;

namespace
{
  enum { MAX_UNITS = 32 };
}

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

  void applyBindings(const array<Binding, MAX_UNITS> bindings, TextureManager &mgr)
  {
    for (size_t i = 0; i < MAX_UNITS; i++)
    {
      applyBinding(i, bindings[i], mgr);
    }
  }


} // namespace



struct TextureManager::Private
{
  unsigned int lowest_unit = 0;
  bool is_active = false;
  array<Binding, MAX_UNITS> bindings;
};


TextureManager::TextureManager(unsigned int lowest_unit) : p(new Private)
{
  p->lowest_unit = lowest_unit;
}

TextureManager::~TextureManager()
{
  delete p;
}


int TextureManager::getTexUnitNum(unsigned int unit) const
{
  assert(p->lowest_unit + unit < MAX_UNITS);

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
  assert(unit < MAX_UNITS);
  Binding &binding = p->bindings[unit];

  binding.texture = texture->getID();
  binding.target= texture->getTarget();

  if (p->is_active)
    applyBinding(unit, binding, *this);
}


} // namespace render_util
