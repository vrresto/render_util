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

#ifndef RENDER_UTIL_SHADER_UTIL_H
#define RENDER_UTIL_SHADER_UTIL_H

#include <render_util/shader.h>
#include <render_util/texture_manager.h>

#include <string>

namespace render_util
{
  ShaderProgramPtr createShaderProgram(const std::string &definition,
    const render_util::TextureManager &tex_mgr,
    const ShaderSearchPath &search_path,
    const std::map<unsigned int, std::string> &attribute_locations = {},
    const ShaderParameters &params = {});
}

#endif
