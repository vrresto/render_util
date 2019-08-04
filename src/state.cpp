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

#include <render_util/state.h>

using namespace render_util::gl_binding;


namespace
{


int getInt(GLenum name)
{
  int value = 0;
  gl::GetIntegerv(name, &value);
  return value;
}


}


namespace render_util
{


const State &State::defaults()
{
  static State s;
  static bool set = false;

  if (!set)
  {
    s.attributes.at(AttributeIndex::FRONT_FACE) = GL_CW;
    s.attributes.at(AttributeIndex::CULL_FACE) = GL_BACK;
    s.attributes.at(AttributeIndex::DEPTH_FUNC) = GL_LEQUAL;
    s.attributes.at(AttributeIndex::BLEND_SRC) = GL_SRC_ALPHA;
    s.attributes.at(AttributeIndex::BLEND_DST) = GL_ONE_MINUS_SRC_ALPHA;
    s.attributes.at(AttributeIndex::DEPTH_MASK) = true;

    s.enables.at(EnableIndex::CULL_FACE) = true;
    s.enables.at(EnableIndex::BLEND) = false;
    s.enables.at(EnableIndex::DEPTH_TEST) = true;
    s.enables.at(EnableIndex::STENCIL_TEST) = false;

    set = true;
  }

  return s;
}


State State::fromCurrent()
{
  State s;

  for (unsigned int i = 0; i < AttributeIndex::MAX; i++)
  {
    auto name = getAttributeNameFromIndex(i);
    s.attributes.at(i) = getInt(name);
  }

  for (unsigned int i = 0; i < EnableIndex::MAX; i++)
  {
    auto name = getEnableNameFromIndex(i);
    s.enables.at(i) = gl::IsEnabled(name);
  }

  return std::move(s);
}


StateModifier::~StateModifier()
{
  restoreAttribute<AttributeIndex::FRONT_FACE>();
  restoreAttribute<AttributeIndex::CULL_FACE>();
  restoreAttribute<AttributeIndex::DEPTH_FUNC>();
  restoreAttribute<AttributeIndex::DEPTH_MASK>();

  restoreEnable(EnableIndex::CULL_FACE);
  restoreEnable(EnableIndex::BLEND);
  restoreEnable(EnableIndex::DEPTH_TEST);

  if (original_state.attributes.at(AttributeIndex::BLEND_SRC) !=
      current_state.attributes.at(AttributeIndex::BLEND_SRC) ||
      original_state.attributes.at(AttributeIndex::BLEND_DST) !=
      current_state.attributes.at(AttributeIndex::BLEND_DST))
  {
    gl::BlendFunc(original_state.attributes.at(AttributeIndex::BLEND_SRC),
                  original_state.attributes.at(AttributeIndex::BLEND_DST));
  }
}


void StateModifier::setDefaults()
{
  static const auto defaults = State::defaults();

  setFrontFace(defaults.attributes.at(AttributeIndex::FRONT_FACE));
  setCullFace(defaults.attributes.at(AttributeIndex::CULL_FACE));
  setDepthFunc(defaults.attributes.at(AttributeIndex::DEPTH_FUNC));
  setDepthMask(defaults.attributes.at(AttributeIndex::DEPTH_MASK));

//     setBlendFunc(defaults.attributes.at(AttributeIndex::BLEND_SRC),
//                  defaults.attributes.at(AttributeIndex::BLEND_DST));

  enableBlend(defaults.enables.at(EnableIndex::BLEND));
  enableCullFace(defaults.enables.at(EnableIndex::CULL_FACE));
  enableDepthTest(defaults.enables.at(EnableIndex::CULL_FACE));
  enableStencilTest(defaults.enables.at(EnableIndex::STENCIL_TEST));
}


}
