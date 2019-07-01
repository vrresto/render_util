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


#include <render_util/text_display.h>

namespace render_util
{


TextDisplay::TextDisplay(TextureManager &tex_mgr, ShaderSearchPath shader_seach_path) :
  m_background(tex_mgr, shader_seach_path)
{
  m_background.setColor(glm::vec4(0, 0, 0, 0.8));
}


void TextDisplay::addLine(const std::string &line, const glm::vec3 &color)
{
  m_max_line_size = std::max(line.size(), m_max_line_size);

  m_lines.push_back(line);
  m_line_colors.push_back(color);
}


void TextDisplay::clear()
{
  m_lines.clear();
  m_line_colors.clear();
  m_max_line_size = 0;
}


void TextDisplay::draw(TextRenderer &renderer, int x, int y)
{
  m_background.draw(x, y, getWidth(), getHeight());

  for (int i = 0; i < m_lines.size(); i++)
  {
    float offset_y = i * char_height;

    auto color = m_line_colors.at(i);

    renderer.SetColor(0,0,0);
    renderer.DrawText(m_lines.at(i), 1, offset_y + 1);

    renderer.SetColor(color.r, color.g, color.b);
    renderer.DrawText(m_lines.at(i), 0, offset_y);
  }
}


int TextDisplay::getWidth()
{
  return m_max_line_size * char_width;
}


int TextDisplay::getHeight()
{
  return m_lines.size() * char_height;
}


}
