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


#ifndef RENDER_UTIL_TEXT_DISPLAY_H
#define RENDER_UTIL_TEXT_DISPLAY_H

#include <render_util/quad_2d.h>
#include <render_util/texture_manager.h>
#include <render_util/shader.h>
#include <text_renderer/text_renderer.h>


namespace render_util
{


struct TextDisplay
{
  static constexpr int char_width = 9;
  static constexpr int char_height = 15;

  Quad2D m_background;
  std::vector<std::string> m_lines;
  std::vector<glm::vec3> m_line_colors;
  size_t m_max_line_size = 0;

public:
  TextDisplay(TextureManager &tex_mgr, ShaderSearchPath shader_seach_path);

  void addLine(const std::string &line = {}, const glm::vec3 &color = glm::vec3(1.0));
  void clear();
  void draw(TextRenderer &renderer, int x, int y);
  int getWidth();
  int getHeight();
};


}

#endif
