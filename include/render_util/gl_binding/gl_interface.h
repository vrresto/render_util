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

#ifndef GL_INTERFACE_H
#define GL_INTERFACE_H

#include <render_util/config.h>

#include <atomic>
#include <cassert>
#include <GL/gl.h>
#include <GL/glext.h>

#ifndef GLAPIENTRY
  #define GLAPIENTRY __stdcall
#endif

namespace render_util::gl_binding
{

  struct GL_Interface
  {
    typedef void* GetProcAddressFunc(const char *name);

    GL_Interface(GetProcAddressFunc *getProcAddress);

  #if ENABLE_GL_DEBUG_CALLBACK
    bool hasError() { return has_error; }
    void clearError() { has_error = false; }
  #else
    static constexpr bool hasError() { return false; }
    static void clearError() {}
  #endif

    #include <gl_binding/_generated/gl_p_proc.inc>

    static GL_Interface *getCurrent() { return s_current; }
    static void setCurrent(GL_Interface *iface);

  private:
  #if ENABLE_GL_DEBUG_CALLBACK
    static void GLAPIENTRY messageCallback(GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam);

    std::atomic_bool has_error = false;
  #endif

    static GL_Interface *s_current;
  };

}

#endif
