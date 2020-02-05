/**
 * Copyright (c) 2019 Jan Lepper
 * Copyright (c) 2017 Eric Bruneton
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

#include "atmosphere_precomputed.h"

#include <render_util/texunits.h>
#include <render_util/physics.h>
#include <util.h>

using namespace atmosphere;
using render_util::Atmosphere;
using render_util::AtmospherePrecomputed;


class render_util::AtmospherePrecomputed::ToneMappingOperator
{
public:
  virtual bool hasParameter(Atmosphere::Parameter) { return false; }
  virtual double getParameter(Atmosphere::Parameter p) { return 0; }
  virtual void setParameter(Atmosphere::Parameter p, double value) {}
  virtual void setUniforms(render_util::ShaderProgramPtr) {}
};


namespace
{


class DefaultToneMappingOperator : public AtmospherePrecomputed::ToneMappingOperator
{
  using Parameter = Atmosphere::Parameter;

public:
  bool hasParameter(Atmosphere::Parameter p) override
  {
    switch (p)
    {
      case Parameter::EXPOSURE:
      case Parameter::SATURATION:
      case Parameter::BRIGHTNESS_CURVE_EXPONENT:
      case Parameter::GAMMA:
        return true;
      default:
        return false;
    }
  }
};


enum class ToneMappingOperatorType
{
  DEFAULT,
};


constexpr double kPi = util::PI;
constexpr double kSunAngularRadius = render_util::physics::SUN_ANGULAR_RADIUS;
constexpr double kSunSolidAngle = kPi * kSunAngularRadius * kSunAngularRadius;
// constexpr double kLengthUnitInMeters = 1000.0;
constexpr double kLengthUnitInMeters = 1.0;

constexpr double kBottomRadius = render_util::physics::EARTH_RADIUS;
constexpr double kTopRadius = kBottomRadius + 80000;

constexpr bool use_half_precision_ = false;
constexpr bool use_constant_solar_spectrum_ = false;
constexpr bool use_ozone_ = true;
constexpr bool use_combined_textures_ = true;
constexpr bool do_white_balance_ = true;
constexpr double MAX_MIE_ANGSTROM_BETA_FACTOR = 10;
constexpr auto TONE_MAPPING_OPERATOR_TYPE = ToneMappingOperatorType::DEFAULT;

// calculated using CIECAM02 according to http://www.magnetkern.de/spektrum.html
constexpr auto RGB_LAMBDAS = glm::dvec3(630.0, 542.0, 454.0);

// calculated using method method "physical" described in http://www.magnetkern.de/spektrum.html
// constexpr auto RGB_LAMBDAS = glm::dvec3(611.36, 548.96, 464.28)

// CIE RGB color space - https://en.wikipedia.org/wiki/CIE_1931_color_space#CIE_RGB_color_space
// constexpr auto RGB_LAMBDAS = glm::dvec3(700, 546.1, 435.8);

// originally used in this model
// constexpr auto RGB_LAMBDAS = glm::dvec3(680.0, 550.0, 440.0);


}


namespace render_util
{


AtmospherePrecomputed::AtmospherePrecomputed(render_util::TextureManager &tex_mgr,
                                             std::string shader_dir, float max_cirrus_albedo,
                                             bool precomputed_luminance,
                                             float haziness_) :
  m_max_cirrus_albedo(max_cirrus_albedo),
  m_use_luminance(precomputed_luminance ? Luminance::PRECOMPUTED : Luminance::APPROXIMATE)
{
  switch (TONE_MAPPING_OPERATOR_TYPE)
  {
    case ToneMappingOperatorType::DEFAULT:
      m_tone_mapping_operator = std::make_unique<DefaultToneMappingOperator>();
      break;
  }

  // Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
  // (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
  // summed and averaged in each bin (e.g. the value for 360nm is the average
  // of the ASTM G-173 values for all wavelengths between 360 and 370nm).
  // Values in W.m^-2.
  constexpr int kLambdaMin = 360;
  constexpr int kLambdaMax = 830;
  constexpr double kSolarIrradiance[48] = {
    1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
    1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
    1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
    1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
    1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
    1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992
  };
  // Values from http://www.iup.uni-bremen.de/gruppen/molspec/databases/
  // referencespectra/o3spectra2011/index.html for 233K, summed and averaged in
  // each bin (e.g. the value for 360nm is the average of the original values
  // for all wavelengths between 360 and 370nm). Values in m^2.
  constexpr double kOzoneCrossSection[48] = {
    1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
    8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
    1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
    4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
    2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
    6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
    2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
  };
  // From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
  constexpr double kDobsonUnit = 2.687e20;
  // Maximum number density of ozone molecules, in m^-3 (computed so at to get
  // 300 Dobson units of ozone - for this we divide 300 DU by the integral of
  // the ozone density profile defined below, which is equal to 15km).
  constexpr double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
  // Wavelength independent solar irradiance "spectrum" (not physically
  // realistic, but was used in the original implementation).
  constexpr double kConstantSolarIrradiance = 1.5;
  constexpr double kRayleigh = 1.24062e-6;
  constexpr double kRayleighScaleHeight = 8000.0;
  constexpr double kMieScaleHeight = 1200.0;
  constexpr double kMieAngstromAlpha = 0.0;
  constexpr double kMieAngstromBeta = 5.328e-3;
  constexpr double kMieSingleScatteringAlbedo = 0.9;
  constexpr double kMiePhaseFunctionG = 0.8;
  constexpr double kGroundAlbedo = 0.1;
  const double max_sun_zenith_angle =
      (use_half_precision_ ? 102.0 : 120.0) / 180.0 * kPi;

  DensityProfileLayer
      rayleigh_layer(0.0, 1.0, -1.0 / kRayleighScaleHeight, 0.0, 0.0);
  DensityProfileLayer mie_layer(0.0, 1.0, -1.0 / kMieScaleHeight, 0.0, 0.0);
  // Density profile increasing linearly from 0 to 1 between 10 and 25km, and
  // decreasing linearly from 1 to 0 between 25 and 40km. This is an approximate
  // profile from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/
  // Documents/Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
  std::vector<DensityProfileLayer> ozone_density;
  ozone_density.push_back(
      DensityProfileLayer(25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0));
  ozone_density.push_back(
      DensityProfileLayer(0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0));

  auto haziness = glm::clamp(static_cast<double>(haziness_), 0.0, 1.0);
  auto mie_angstrom_beta_factor = glm::mix(1.0, MAX_MIE_ANGSTROM_BETA_FACTOR, haziness);

  std::vector<double> wavelengths;
  std::vector<double> solar_irradiance;
  std::vector<double> rayleigh_scattering;
  std::vector<double> mie_scattering;
  std::vector<double> mie_extinction;
  std::vector<double> absorption_extinction;
  std::vector<double> ground_albedo;
  for (int l = kLambdaMin; l <= kLambdaMax; l += 10) {
    double lambda = static_cast<double>(l) * 1e-3;  // micro-meters
    double mie = mie_angstrom_beta_factor * kMieAngstromBeta / kMieScaleHeight
                 * pow(lambda, -kMieAngstromAlpha);
    wavelengths.push_back(l);
    if (use_constant_solar_spectrum_) {
      solar_irradiance.push_back(kConstantSolarIrradiance);
    } else {
      solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
    }
    rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
    mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
    mie_extinction.push_back(mie);
    absorption_extinction.push_back(use_ozone_ ?
        kMaxOzoneNumberDensity * kOzoneCrossSection[(l - kLambdaMin) / 10] :
        0.0);
    ground_albedo.push_back(kGroundAlbedo);
  }

  m_model = std::unique_ptr<atmosphere::Model>(new atmosphere::Model(
      RGB_LAMBDAS,
      wavelengths, solar_irradiance, kSunAngularRadius,
      kBottomRadius, kTopRadius,
      {rayleigh_layer}, rayleigh_scattering,
      {mie_layer}, mie_scattering, mie_extinction, kMiePhaseFunctionG,
      ozone_density, absorption_extinction, ground_albedo, max_sun_zenith_angle,
      kLengthUnitInMeters, m_use_luminance == Luminance::PRECOMPUTED ? 15 : 3,
      use_combined_textures_, use_half_precision_, shader_dir + "/" + getShaderPath(),
      tex_mgr, false, 0));

  m_model->Init(6);

  {
    using namespace render_util;
    m_transmittance_texture_unit = tex_mgr.getTexUnitNum(TEXUNIT_ATMOSPHERE0);
    m_scattering_texture_unit = tex_mgr.getTexUnitNum(TEXUNIT_ATMOSPHERE1);
    m_irradiance_texture_unit = tex_mgr.getTexUnitNum(TEXUNIT_ATMOSPHERE2);
    m_single_mie_scattering_texture_unit = tex_mgr.getTexUnitNum(TEXUNIT_ATMOSPHERE3);
  }

  glm::dvec3 white_point(1.0);
  if (do_white_balance_)
  {
    Model::ConvertSpectrumToLinearSrgb(wavelengths, solar_irradiance,
        &white_point.r, &white_point.g, &white_point.b);

    white_point /= (white_point.r + white_point.g + white_point.b) / 3.0;
  }
  m_white_point = white_point;
}


AtmospherePrecomputed::~AtmospherePrecomputed() {}


ShaderParameters AtmospherePrecomputed::getShaderParameters()
{
  auto p = m_model->getShaderParameters();
  p.set("use_luminance", m_use_luminance != Luminance::NONE);
  p.set("use_hdr", true);
  p.set("max_cirrus_albedo", m_max_cirrus_albedo);

  switch (TONE_MAPPING_OPERATOR_TYPE)
  {
    case ToneMappingOperatorType::DEFAULT:
      p.set("use_default_tone_mapping", true);
      break;
  }

  return p;
}


void AtmospherePrecomputed::setUniforms(ShaderProgramPtr program)
{
  m_model->SetProgramUniforms(program,
    m_transmittance_texture_unit,
    m_scattering_texture_unit,
    m_irradiance_texture_unit,
    m_single_mie_scattering_texture_unit);

  program->setUniform<float>("exposure",
                             m_use_luminance != Luminance::NONE ? m_exposure * 1e-5 : m_exposure);

  program->setUniform("gamma", m_gamma);
  program->setUniform("saturation", m_saturation);
  program->setUniform("brightness_curve_exponent", m_brightness_curve_exponent);
  program->setUniform("texture_brightness", m_texture_brightness);
  program->setUniform("texture_brightness_curve_exponent", m_texture_brightness_curve_exponent);
  program->setUniform("texture_saturation", m_texture_saturation);
  program->setUniform("blue_saturation", m_blue_saturation);

  program->setUniform("white_point", glm::vec3(m_white_point));
  m_tone_mapping_operator->setUniforms(program);
}


bool AtmospherePrecomputed::hasParameter(Parameter p)
{
  switch (p)
  {
    case Parameter::TEXTURE_BRIGHTNESS:
    case Parameter::TEXTURE_SATURATION:
    case Parameter::TEXTURE_BRIGHTNESS_CURVE_EXPONENT:
//     case Parameter::BLUE_SATURATION:
      return true;
    default:
      return m_tone_mapping_operator->hasParameter(p);
  }
}


double AtmospherePrecomputed::getParameter(Parameter p)
{
  switch (p)
  {
    case Parameter::EXPOSURE:
      return m_exposure;
    case Parameter::SATURATION:
      return m_saturation;
    case Parameter::BRIGHTNESS_CURVE_EXPONENT:
      return m_brightness_curve_exponent;
    case Parameter::TEXTURE_BRIGHTNESS:
      return m_texture_brightness;
    case Parameter::TEXTURE_BRIGHTNESS_CURVE_EXPONENT:
      return m_texture_brightness_curve_exponent;
    case Parameter::TEXTURE_SATURATION:
      return m_texture_saturation;
    case Parameter::BLUE_SATURATION:
      return m_blue_saturation;
    case Parameter::GAMMA:
      return m_gamma;
    default:
      return m_tone_mapping_operator->getParameter(p);
  }
}


void AtmospherePrecomputed::setParameter(Parameter p, double value)
{
  switch (p)
  {
    case Parameter::EXPOSURE:
      m_exposure = std::max(0.0, value);
      break;
    case Parameter::SATURATION:
      m_saturation = value;
      break;
    case Parameter::BRIGHTNESS_CURVE_EXPONENT:
      m_brightness_curve_exponent = value;
      break;
    case Parameter::TEXTURE_BRIGHTNESS:
      m_texture_brightness = std::max(0.0, value);
      break;
    case Parameter::TEXTURE_BRIGHTNESS_CURVE_EXPONENT:
      m_texture_brightness_curve_exponent = value;
      break;
    case Parameter::TEXTURE_SATURATION:
      m_texture_saturation = value;
      break;
    case Parameter::BLUE_SATURATION:
      m_blue_saturation = value;
      break;
    case Parameter::GAMMA:
      m_gamma = std::max(1.0, value);
      break;
    default:
      m_tone_mapping_operator->setParameter(p, value);
  }
}


}
