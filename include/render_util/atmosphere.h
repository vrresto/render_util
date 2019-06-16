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

#ifndef RENDER_UTIL_ATMOSPHERE_H
#define RENDER_UTIL_ATMOSPHERE_H

#include <render_util/shader.h>
#include <render_util/camera.h>

#include <string>

namespace render_util
{


class Atmosphere
{
public:
  virtual std::string getShaderPath() { return "atmosphere_simple"; }
  virtual ShaderParameters getShaderParameters() { return {}; }
  virtual void setUniforms(ShaderProgramPtr program, const Camera&, glm::vec3 sun_direction) {}
};


}

#endif
