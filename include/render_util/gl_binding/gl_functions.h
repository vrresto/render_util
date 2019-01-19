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

#ifndef GL_FUNCTIONS2_H
#define GL_FUNCTIONS2_H

#include <cassert>
#include <cstdio>

#include <render_util/gl_binding/gl_interface.h>
#include <render_util/gl_binding/gl_binding.h>

namespace render_util::gl_binding::gl
{
  #include <gl_binding/_generated/gl_inline_forwards.inc>
}

#define FORCE_CHECK_GL_ERROR() \
{ \
  render_util::gl_binding::gl::Finish(); \
  auto err = render_util::gl_binding::gl::GetError(); \
  if (err != GL_NO_ERROR) \
  { \
    printf("gl error: %s\n", render_util::gl_binding::getGLErrorString(err)); \
  } \
  assert(err == GL_NO_ERROR); \
}

#if RENDER_UTIL_ENABLE_DEBUG
  #define CHECK_GL_ERROR() FORCE_CHECK_GL_ERROR()
#else
  #define CHECK_GL_ERROR() {}
#endif

#endif
