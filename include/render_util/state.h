/**
 *    Rendering utilities
 *    Copyright (C) 2019 Jan Lepper
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

#include <render_util/gl_binding/gl_functions.h>

#include <array>
#include <variant>

#ifndef RENDER_UTIL_STATE_H
#define RENDER_UTIL_STATE_H


namespace render_util
{


struct State
{
  struct EnableIndex
  {
    enum Enum : unsigned int
    {
      CULL_FACE = 0,
      BLEND,
      DEPTH_TEST,
      STENCIL_TEST,
      ALPHA_TEST,
      MAX,
    };
  };

  std::array<bool, EnableIndex::MAX> enables;
  GLenum front_face = 0;
  GLenum cull_face = 0;
  GLenum depth_func = 0;
  bool depth_mask = 0;
  GLenum blend_src = 0;
  GLenum blend_dst = 0;
  GLenum alpha_test_func = 0;
  GLclampf alpha_test_ref = 0;

  static State fromCurrent();

  static GLenum getEnableNameFromIndex(unsigned int index)
  {
    switch (index)
    {
      case EnableIndex::CULL_FACE:
        return GL_CULL_FACE;
      case EnableIndex::BLEND:
        return GL_BLEND;
      case EnableIndex::DEPTH_TEST:
        return GL_DEPTH_TEST;
      case EnableIndex::STENCIL_TEST:
        return GL_STENCIL_TEST;
      case EnableIndex::ALPHA_TEST:
        return GL_ALPHA_TEST;
      default:
        assert(0);
        abort();
    }
  }
};


struct StateModifier
{
  using EnableIndex = State::EnableIndex;

  StateModifier(const State &original_state) :
    original_state(original_state),
    current_state(original_state)
  {
  }

  StateModifier(const StateModifier &prev) :
    original_state(prev.current_state),
    current_state(prev.current_state)
  {
  }

  ~StateModifier();

  void setDefaults();

  template <auto Setter, typename T>
  void set(T &attr, T value)
  {
    if (attr != value)
    {
      attr = value;
      Setter(value);
    }
  }

  void setBlendFunc(GLenum sfactor, GLenum dfactor)
  {
    if (current_state.blend_src != sfactor ||
        current_state.blend_dst != dfactor)
    {
      current_state.blend_src = sfactor;
      current_state.blend_dst = dfactor;
      gl_binding::gl::BlendFunc(sfactor, dfactor);
    }
  }

  void setAlphaFunc(GLenum func, GLclampf ref)
  {
    if (current_state.alpha_test_func != func ||
        current_state.alpha_test_ref != ref)
    {
      current_state.alpha_test_func = func;
      current_state.alpha_test_ref = ref;
      gl_binding::gl::AlphaFunc(func, ref);
    }
  }

  void setFrontFace(GLenum value)
  {
    set<gl_binding::gl::FrontFace>(current_state.front_face, value);
  }

  void setCullFace(GLenum value)
  {
    set<gl_binding::gl::CullFace>(current_state.cull_face, value);
  }

  void setDepthFunc(GLenum value)
  {
    set<gl_binding::gl::DepthFunc>(current_state.depth_func, value);
  }

  void setDepthMask(bool value)
  {
    set<gl_binding::gl::DepthMask>(current_state.depth_mask, value);
  }

  void enableCullFace(bool value)
  {
    enable(EnableIndex::CULL_FACE, value);
  }

  void enableBlend(bool value)
  {
    enable(EnableIndex::BLEND, value);
  }

  void enableDepthTest(bool value)
  {
    enable(EnableIndex::DEPTH_TEST, value);
  }

  void enableStencilTest(bool value)
  {
    enable(EnableIndex::STENCIL_TEST, value);
  }

  void enableAlphaTest(bool value)
  {
    enable(EnableIndex::ALPHA_TEST, value);
  }

private:
  void enable(EnableIndex::Enum index, bool value)
  {
    using namespace render_util::gl_binding;

    if (current_state.enables.at(index) != value)
    {
      current_state.enables.at(index) = value;
      auto name = State::getEnableNameFromIndex(index);

      if (value)
        gl::Enable(name);
      else
        gl::Disable(name);
    }
  }

  void restoreEnable(EnableIndex::Enum index)
  {
    enable(index, original_state.enables.at(index));
  }

  const State &original_state;
  State current_state;
};


}

#endif
