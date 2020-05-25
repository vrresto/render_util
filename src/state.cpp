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


void get(GLenum name, int *value)
{
  gl::GetIntegerv(name, value);
}

void get(GLenum name, unsigned int *value)
{
  int v = 0;
  gl::GetIntegerv(name, &v);
  *value = v;
}

void get(GLenum name, float *value)
{
  gl::GetFloatv(name, value);
}

void get(GLenum name, bool *value)
{
  int v = 0;
  gl::GetIntegerv(name, &v);
  *value = v;
}


}


namespace render_util
{


State State::fromCurrent()
{
  State s;

  #define LOAD(attr, name) { get(name, &s.attr); }
  LOAD(front_face, GL_FRONT_FACE);
  LOAD(cull_face, GL_CULL_FACE);
  LOAD(depth_func, GL_DEPTH_FUNC);
  LOAD(depth_mask, GL_DEPTH_WRITEMASK);
  LOAD(blend_src, GL_BLEND_SRC);
  LOAD(blend_dst, GL_BLEND_DST);
  LOAD(alpha_test_func, GL_ALPHA_TEST_FUNC);
  LOAD(alpha_test_ref, GL_ALPHA_TEST_REF);
  #undef LOAD

  for (unsigned int i = 0; i < EnableIndex::MAX; i++)
  {
    auto name = getEnableNameFromIndex(i);
    s.enables.at(i) = gl::IsEnabled(name);
  }

  return s;
}


StateModifier::~StateModifier()
{
  setFrontFace(original_state.front_face);
  setCullFace(original_state.cull_face);
  setDepthFunc(original_state.depth_func);
  setDepthMask(original_state.depth_mask);
  setBlendFunc(original_state.blend_src, original_state.blend_dst);
  setAlphaFunc(original_state.alpha_test_func, original_state.alpha_test_ref);

  restoreEnable(EnableIndex::CULL_FACE);
  restoreEnable(EnableIndex::BLEND);
  restoreEnable(EnableIndex::DEPTH_TEST);
  restoreEnable(EnableIndex::STENCIL_TEST);
  restoreEnable(EnableIndex::ALPHA_TEST);
}


void StateModifier::setDefaults()
{
  setFrontFace(GL_CW);
  setCullFace(GL_BACK);
  setDepthFunc(GL_LEQUAL);
  setDepthMask(true);
  setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  setAlphaFunc(GL_ALWAYS, 0);

  enableBlend(false);
  enableCullFace(false);
  enableDepthTest(false);
  enableStencilTest(false);
  enableAlphaTest(false);
}


}
