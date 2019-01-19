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

#include <GL/gl.h>
#include <GL/glext.h>

#include <render_util/gl_binding/gl_interface.h>

#define PROC_INIT(type, proc) proc = (type) getProcAddress(#proc);

namespace render_util::gl_binding
{
  GL_Interface *GL_Interface::s_current = 0;

  GL_Interface::GL_Interface(GetProcAddressFunc *getProcAddress)
  {
    #include "gl_binding/_generated/gl_p_proc_init.inc"
  }

  void GL_Interface::setCurrent(GL_Interface *iface)
  {
    s_current = iface;
  }

}
