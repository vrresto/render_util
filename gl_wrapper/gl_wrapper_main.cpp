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

#include <gl_wrapper/gl_wrapper.h>

#include <cstdlib>
#include <cassert>
#include <GL/gl.h>

namespace
{
  const gl_wrapper::GL_Interface *current_gl_interface = 0;
}

namespace gl_wrapper
{

  void setCurrent_GL_interface(const GL_Interface *interface)
  {
    current_gl_interface = interface;
  }
  
  const GL_Interface *current_GL_Interface()
  {
    assert(current_gl_interface);
    return current_gl_interface;
  }

  const char *getGLErrorString(unsigned int code)
  {
    switch (code)
    {
      case GL_NO_ERROR:
        return "GL_NO_ERROR";
      case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
      case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
      case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
      case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW";
      case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW";
      case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
      default:
        return "unknown error";
    }
  }

}
