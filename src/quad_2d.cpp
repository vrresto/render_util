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


#include <render_util/quad_2d.h>
#include <render_util/shader_util.h>
#include <render_util/globals.h>
#include <render_util/gl_binding/gl_functions.h>

#include <glm/glm.hpp>


using namespace render_util::gl_binding;
using namespace glm;


namespace render_util
{


Quad2D::Quad2D(TextureManager &tex_mgr, const ShaderSearchPath &shader_search_path)
{
  shader = createShaderProgram("quad_2d", tex_mgr, shader_search_path);
  assert(shader->isValid());
}


void Quad2D::setColor(glm::vec4 color)
{
  shader->setUniform("color", color);
}


void Quad2D::draw(int x_, int y_, int width_, int height_)
{
  auto old_program = getCurrentGLContext()->getCurrentProgram();
  getCurrentGLContext()->setCurrentProgram(shader);

  vec2 pos(x_, y_);
  vec2 size(width_, height_);

  int viewport[4];
  gl::GetIntegerv(GL_VIEWPORT, viewport);

  vec2 viewport_size(viewport[2], viewport[3]);

  vec2 pos_ndc = pos / viewport_size;
  pos_ndc.y = 1.f - pos_ndc.y;
  pos_ndc = (2.f * pos_ndc) - vec2(1);

  vec2 size_ndc = 2.f * (size / viewport_size);

  gl::Begin(GL_QUADS);
  gl::Vertex3f(pos_ndc.x, pos_ndc.y, 0);
  gl::Vertex3f(pos_ndc.x + size_ndc.x, pos_ndc.y, 0);
  gl::Vertex3f(pos_ndc.x + size_ndc.x, pos_ndc.y - size_ndc.y, 0);
  gl::Vertex3f(pos_ndc.x, pos_ndc.y - size_ndc.y, 0);
  gl::End();

  getCurrentGLContext()->setCurrentProgram(old_program);
}


}
