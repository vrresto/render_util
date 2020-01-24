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
public:
  class ToneMappingOperator;

private:
  std::unique_ptr<atmosphere::Model> m_model;
  std::unique_ptr<ToneMappingOperator> m_tone_mapping_operator;

  unsigned int m_transmittance_texture_unit = 0;
  unsigned int m_scattering_texture_unit = 0;
  unsigned int m_irradiance_texture_unit = 0;
  unsigned int m_single_mie_scattering_texture_unit = 0;
  glm::dvec3 m_white_point = glm::dvec3(1);

  float m_gamma = 2.2;
  float m_exposure = 20;
  float m_saturation = 1.0;
  float m_brightness_curve_exponent = 1.3;
  float m_texture_brightness = 0.2;
  float m_texture_brightness_curve_exponent = 1.0;
  float m_texture_saturation = 1.0;
  float m_blue_saturation = 1.0;

  float m_max_cirrus_albedo = 1.0;

public:
  AtmospherePrecomputed(render_util::TextureManager &tex_mgr, std::string shader_dir,
                        float max_cirrus_albedo);
  ~AtmospherePrecomputed();

  std::string getShaderPath() override { return "atmosphere_precomputed"; }
  ShaderParameters getShaderParameters() override;
  void setUniforms(ShaderProgramPtr) override;

  bool hasParameter(Parameter) override;
  double getParameter(Parameter) override;
  void setParameter(Parameter, double) override;
};


}

#endif
