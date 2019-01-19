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

#include <render_util/gl_context.h>
#include <render_util/gl_binding/gl_functions.h>

using namespace render_util::gl_binding;

void render_util::GLContext::setCurrentProgram(ShaderProgramPtr program)
{
  m_current_program = program;

  if (m_current_program)
  {
    assert(m_current_program->isValid());
    gl::UseProgram(m_current_program->getId());
  }
  else
  {
    gl::UseProgram(0);
  }
}
