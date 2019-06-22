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

#ifndef RENDER_UTIL_ATMOSPHERE_PRECOMPUTED_H
#define RENDER_UTIL_ATMOSPHERE_PRECOMPUTED_H

#include <render_util/atmosphere.h>
#include <precomputed_precomputed_atmospheric_scattering/atmosphere/model.h>

#include <string>

namespace render_util
{


class AtmospherePrecomputed : public Atmosphere
{
  std::unique_ptr<atmosphere::Model> m_model;

  unsigned int m_transmittance_texture_unit = 0;
  unsigned int m_scattering_texture_unit = 0;
  unsigned int m_irradiance_texture_unit = 0;
  unsigned int m_single_mie_scattering_texture_unit = 0;
  glm::dvec3 m_white_point = glm::dvec3(1);

  static constexpr float gamma = 2.2;
  static constexpr float exposure = 20;
  static constexpr float texture_brightness = 0.2;

public:
  AtmospherePrecomputed(render_util::TextureManager &tex_mgr, std::string shader_dir);

  std::string getShaderPath() override { return "atmosphere_precomputed"; }
  ShaderParameters getShaderParameters() override;
  void setUniforms(ShaderProgramPtr, const Camera&) override;
};


}

#endif
