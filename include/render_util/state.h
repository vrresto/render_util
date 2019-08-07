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

#ifndef RENDER_UTIL_STATE_H
#define RENDER_UTIL_STATE_H


namespace render_util
{


struct State
{
  struct AttributeIndex
  {
    enum Enum : unsigned int
    {
      FRONT_FACE = 0,
      CULL_FACE ,
      DEPTH_FUNC,
      BLEND_SRC,
      BLEND_DST,
      DEPTH_MASK,
      MAX,
    };
  };

  struct EnableIndex
  {
    enum Enum : unsigned int
    {
      CULL_FACE = 0,
      BLEND,
      DEPTH_TEST,
      STENCIL_TEST,
      MAX,
    };
  };

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
      default:
        assert(0);
        abort();
    }
  }

  static GLenum getAttributeNameFromIndex(unsigned int index)
  {
    switch (index)
    {
      case AttributeIndex::FRONT_FACE:
        return GL_FRONT_FACE;
      case AttributeIndex::CULL_FACE:
        return GL_CULL_FACE_MODE;
      case AttributeIndex::DEPTH_FUNC:
        return GL_DEPTH_FUNC;
      case AttributeIndex::DEPTH_MASK:
        return GL_DEPTH_WRITEMASK;
      case AttributeIndex::BLEND_SRC:
        return GL_BLEND_SRC;
      case AttributeIndex::BLEND_DST:
        return GL_BLEND_DST;
      default:
        assert(0);
        abort();
    }
  }

  static const State &defaults();
  static State fromCurrent();

  std::array<int, AttributeIndex::MAX> attributes;
  std::array<bool, EnableIndex::MAX> enables;

private:
  State() {}
};


struct StateModifier
{
  using AttributeIndex = State::AttributeIndex;
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

  void setBlendFunc(GLenum sfactor, GLenum dfactor)
  {
    using namespace render_util::gl_binding;

    if (current_state.attributes.at(AttributeIndex::BLEND_SRC) != sfactor ||
        current_state.attributes.at(AttributeIndex::BLEND_DST) != dfactor)
    {
      current_state.attributes.at(AttributeIndex::BLEND_SRC) = sfactor;
      current_state.attributes.at(AttributeIndex::BLEND_DST) = dfactor;
      gl::BlendFunc(sfactor, dfactor);
    }
  }

  void setFrontFace(GLenum value)
  {
    setAttribute<AttributeIndex::FRONT_FACE>(value);
  }

  void setCullFace(GLenum value)
  {
    setAttribute<AttributeIndex::CULL_FACE>(value);
  }

  void setDepthFunc(GLenum value)
  {
    setAttribute<AttributeIndex::DEPTH_FUNC>(value);
  }

  void setDepthMask(bool value)
  {
    setAttribute<AttributeIndex::DEPTH_MASK>(value);
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

private:
  template <auto T>
  struct AttributeSetter
  {
    template <typename... Args>
    static auto set(Args&&... args) -> decltype(T(std::forward<Args>(args)...))
    {
      return T(std::forward<Args>(args)...);
    }
  };

  template <AttributeIndex::Enum T>
  struct Attribute
  {
  };

  template <AttributeIndex::Enum Index, typename T>
  void setAttribute(T value)
  {
    if (current_state.attributes.at(Index) != value)
    {
      current_state.attributes.at(Index) = value;
      Attribute<Index>::set(value);
    }
  }

  template<AttributeIndex::Enum T>
  void restoreAttribute()
  {
    setAttribute<T>(original_state.attributes.at(T));
  }

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


template <>
struct StateModifier::Attribute<StateModifier::AttributeIndex::FRONT_FACE> :
  public StateModifier::AttributeSetter<render_util::gl_binding::gl::FrontFace>
{
};


template <>
struct StateModifier::Attribute<StateModifier::AttributeIndex::CULL_FACE>
 : public StateModifier::AttributeSetter <render_util::gl_binding::gl::CullFace>
{
};


template <>
struct StateModifier::Attribute<StateModifier::AttributeIndex::DEPTH_FUNC>
 : public StateModifier::AttributeSetter <render_util::gl_binding::gl::DepthFunc>
{
};


template <>
struct StateModifier::Attribute<StateModifier::AttributeIndex::DEPTH_MASK>
 : public StateModifier::AttributeSetter <render_util::gl_binding::gl::DepthMask>
{
};


}

#endif
