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
#include <render_util/texture_manager.h>
#include <render_util/camera.h>

#include <string>

namespace render_util
{


struct AtmosphereCreationParameters
{
  float max_cirrus_albedo = 1.0;
  bool precomputed_luminance = false;
  float haziness = 0;
  bool single_mie_horizon_hack = false;
};


class Atmosphere
{
public:
  enum Type
  {
    DEFAULT,
    PRECOMPUTED
  };

  enum class Parameter
  {
    EXPOSURE,
    SATURATION,
    BRIGHTNESS_CURVE_EXPONENT,
    TEXTURE_BRIGHTNESS,
    TEXTURE_BRIGHTNESS_CURVE_EXPONENT,
    TEXTURE_SATURATION,
    BLUE_SATURATION,
    GAMMA,
    UNCHARTED2_A,
    UNCHARTED2_B,
    UNCHARTED2_C,
    UNCHARTED2_D,
    UNCHARTED2_E,
    UNCHARTED2_F,
    UNCHARTED2_W
  };

  virtual std::string getShaderPath() { return "atmosphere_default"; }
  virtual ShaderParameters getShaderParameters() { return {}; }
  virtual void setUniforms(ShaderProgramPtr program) {}

  virtual bool hasParameter(Parameter) { return false; }
  virtual double getParameter(Parameter) { return 0; }
  virtual void setParameter(Parameter, double) {}
};


std::unique_ptr<Atmosphere> createAtmosphere(Atmosphere::Type,
                                             render_util::TextureManager&, std::string shader_dir,
                                             const AtmosphereCreationParameters&);


}

#endif
