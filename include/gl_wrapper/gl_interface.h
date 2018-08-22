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

#include <GL/gl.h>
#include <GL/glext.h>

#ifndef GLAPIENTRY
  #define GLAPIENTRY __stdcall
#endif

namespace gl_wrapper
{

  struct GL_Interface
  {
    typedef void* GetProcAddressFunc(const char *name);

    GL_Interface(GetProcAddressFunc *getProcAddress);

    #include <gl_wrapper/_generated/gl_p_proc.inc>

    static GL_Interface *getCurrent() { return s_current; }
    static void setCurrent(GL_Interface *iface);

  private:
    static GL_Interface *s_current;
  };

}

#endif
