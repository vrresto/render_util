/**
 * Copyright (c) 2017 Ruslan Kabatsayev
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "text_renderer.h"
#include <render_util/gl_binding/gl_functions.h>

#include <vector>

using namespace render_util::gl_binding;

namespace {

#include "font.inc"

const char kVertexShader[] = R"(
    #version 330
    uniform mat4 clip_from_model;
    uniform mat3x2 texture_coord_from_model;
    layout(location = 0) in vec4 vertex;
    out vec2 texture_coord;
    void main() {
      gl_Position = clip_from_model * vertex;
      texture_coord = texture_coord_from_model * vertex.xyw;
    })";

const char kFragmentShader[] = R"(
    #version 330
    uniform vec3 text_color;
    uniform sampler2D font_sampler;
    in vec2 texture_coord;
    layout(location = 0) out vec4 color;
    void main() {
      color = vec4(text_color, 1) * texture(font_sampler, texture_coord).rrrr;
    })";

}  // anonymous namespace

void TextRenderer::SetupTexture() {
  gl::GenTextures(1, &font_texture_);

  // Avoid interfering with caller's assumptions.
  gl::ActiveTexture(GL_TEXTURE0);
  GLint old_texture;
  gl::GetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture);

  gl::BindTexture(GL_TEXTURE_2D, font_texture_);
  gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RED, font.atlas_width, font.atlas_height,
               0, GL_RED, GL_UNSIGNED_BYTE, font.data.data());
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  gl::BindTexture(GL_TEXTURE_2D, old_texture);
}

void TextRenderer::SetupBuffers() {
  gl::GenVertexArrays(1, &char_vao_);
  gl::BindVertexArray(char_vao_);
  gl::GenBuffers(1, &char_vbo_);
  gl::BindBuffer(GL_ARRAY_BUFFER, char_vbo_);
  const GLfloat vertices[] = {
    0, 0,
    1, 0,
    0, 1,
    1, 1,
  };
  gl::BufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
  constexpr GLuint kAttribIndex = 0;
  constexpr int kCoordsPerVertex = 2;
  gl::VertexAttribPointer(kAttribIndex, kCoordsPerVertex, GL_FLOAT, false, 0, 0);
  gl::EnableVertexAttribArray(kAttribIndex);

  gl::BindBuffer(GL_ARRAY_BUFFER, 0);
  gl::BindVertexArray(0);
}

void TextRenderer::SetupProgram() {
  const auto vertex_shader = gl::CreateShader(GL_VERTEX_SHADER);
  const char* const vertex_shader_source = kVertexShader;
  gl::ShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
  gl::CompileShader(vertex_shader);
  const auto fragment_shader = gl::CreateShader(GL_FRAGMENT_SHADER);
  const char* const fragment_shader_source = kFragmentShader;
  gl::ShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
  gl::CompileShader(fragment_shader);
  program_ = gl::CreateProgram();
  gl::AttachShader(program_, vertex_shader);
  gl::AttachShader(program_, fragment_shader);
  gl::LinkProgram(program_);
  gl::DetachShader(program_, fragment_shader);
  gl::DeleteShader(fragment_shader);
  gl::DetachShader(program_, vertex_shader);
  gl::DeleteShader(vertex_shader);
}

void TextRenderer::DrawChar(char c, int x, int y,
                            int viewport_width, int viewport_height) {
  if (c < 0x20 || c > 0x7e) {
    c = '?';
  }

  const GLfloat char_width = font.char_width;
  const GLfloat char_height = font.char_height;
  {
    const GLfloat scale_x = char_width / font.atlas_width;
    const GLfloat scale_y = -char_height / font.atlas_height;
    const int characters_per_line = font.atlas_width / font.char_width;
    const GLfloat translate_x =
        (c % characters_per_line) * char_width / font.atlas_width;
    const GLfloat translate_y =
        (c / characters_per_line - 1) * char_height / font.atlas_height;
    const GLfloat texture_coord_from_model[] = {
      scale_x, 0, translate_x,
      0, scale_y, translate_y
    };
    gl::UniformMatrix3x2fv(
        gl::GetUniformLocation(program_, "texture_coord_from_model"),
        1, true, texture_coord_from_model);
  }
  {
    const GLfloat scale_x = 2 * char_width / viewport_width;
    const GLfloat scale_y = 2 * char_height / viewport_height;
    const GLfloat translate_x = (2.f * x) / viewport_width - 1;
    const GLfloat translate_y = (2.f * y) / viewport_height - 1;
    const GLfloat clip_from_model[] = {
      scale_x, 0, 0, translate_x,
      0, scale_y, 0, translate_y,
      0, 0, 1, 0,
      0, 0, 0, 1
    };
    gl::UniformMatrix4fv(gl::GetUniformLocation(program_, "clip_from_model"),
                       1, true, clip_from_model);
  }

  gl::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

TextRenderer::TextRenderer() {
  SetupTexture();
  SetupBuffers();
  SetupProgram();

  SetColor(1, 1, 1);
}

TextRenderer::~TextRenderer() {
  gl::DeleteProgram(program_);
  gl::DeleteBuffers(1, &char_vbo_);
  gl::DeleteVertexArrays(1, &char_vao_);
  gl::DeleteTextures(1, &font_texture_);
}

void TextRenderer::SetColor(float r, float g, float b) {
  color_[0] = r;
  color_[1] = g;
  color_[2] = b;
  GLint old_program;
  gl::GetIntegerv(GL_CURRENT_PROGRAM, &old_program);
  gl::UseProgram(program_);
  gl::Uniform3fv(gl::GetUniformLocation(program_, "text_color"), 1, color_);
  gl::UseProgram(old_program);
}

void TextRenderer::DrawText(const std::string& text, int left, int top) {
  GLint viewport[4];
  gl::GetIntegerv(GL_VIEWPORT, viewport);

  // Avoid interfering with caller's assumptions.
  GLint old_program;
  gl::GetIntegerv(GL_CURRENT_PROGRAM, &old_program);
  gl::ActiveTexture(GL_TEXTURE0);
  GLint old_texture;
  gl::GetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture);

  gl::BindVertexArray(char_vao_);
  gl::UseProgram(program_);

  gl::ActiveTexture(GL_TEXTURE0);
  gl::BindTexture(GL_TEXTURE_2D, font_texture_);
  gl::Uniform1i(gl::GetUniformLocation(program_, "font_sampler"), 0);

  gl::Enable(GL_BLEND);
  gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  int x = left;
  int y = viewport[3] - top - font.char_height;
  for (char c : text) {
    switch (c) {
      case ' ':
        break;
      case '\n':
        x = left;
        y -= font.char_height + 1;
        continue;
      default:
        DrawChar(c, x, y, viewport[2], viewport[3]);
        break;
    }
    x += font.char_width;
  }

  gl::Disable(GL_BLEND);
  gl::BindVertexArray(0);

  gl::BindTexture(GL_TEXTURE_2D, old_texture);
  gl::UseProgram(old_program);
}
