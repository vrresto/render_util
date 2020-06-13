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


#include <render_util/gl_binding/gl_binding.h>
#include <log.h>

#include <stdexcept>
#include <GL/gl.h>
#include <GL/glext.h>

#include <render_util/gl_binding/gl_interface.h>


namespace render_util::gl_binding
{
  GL_Interface *GL_Interface::s_current = 0;


  GL_Interface::GL_Interface(GetProcAddressFunc *getProcAddress)
  {
    auto get_proc_address = [getProcAddress] (const char *name)
    {
      auto addr = getProcAddress(name);

      if (!addr)
        throw std::runtime_error(std::string("GL procedure is missing: ") + name);

      return addr;
    };

    #include "gl_binding/_generated/gl_p_proc_init.inc"

    auto check_error = [this] ()
    {
      this->Finish();
      auto err = this->GetError();
      if (err != GL_NO_ERROR)
      {
        LOG_ERROR << "GL error: " << render_util::gl_binding::getGLErrorString(err) << std::endl;
        abort();
      }
    };

    check_error();

    #if ENABLE_GL_DEBUG_CALLBACK
    LOG_INFO << "Enabling GL debug message callback." << std::endl;
    this->DebugMessageCallback(messageCallback, this);
    this->DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, false);
    this->DebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, nullptr, true);
    this->DebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, true);
    this->Enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    this->Enable(GL_DEBUG_OUTPUT);
    check_error();
    #endif
  }


  void GL_Interface::setCurrent(GL_Interface *iface)
  {
    s_current = iface;
  }

  #if ENABLE_GL_DEBUG_CALLBACK
  void GLAPIENTRY GL_Interface::messageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
  {
    if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
    {
      LOG_ERROR << message << std::endl;

      auto iface = (GL_Interface*)userParam;
      iface->has_error = true;
    }
    else
    {
      LOG_WARNING << message << std::endl;
    }
  }
  #endif
}
