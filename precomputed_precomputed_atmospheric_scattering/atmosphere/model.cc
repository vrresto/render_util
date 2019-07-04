/**
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

/*<h2>atmosphere/model.cc</h2>

<p>This file implements the <a href="model.h.html">API of our atmosphere
model</a>. Its main role is to precompute the transmittance, scattering and
irradiance textures. The GLSL functions to precompute them are provided in
<a href="functions.glsl.html">functions.glsl</a>, but they are not sufficient.
They must be used in fully functional shaders and programs, and these programs
must be called in the correct order, with the correct input and output textures
(via framebuffer objects), to precompute each scattering order in sequence, as
described in Algorithm 4.1 of
<a href="https://hal.inria.fr/inria-00288758/en">our paper</a>. This is the role
of the following C++ code.
*/


#include "model.h"

#include "constants.h"
#include <render_util/shader_util.h>
#include <render_util/image_loader.h>

#include <glm/gtc/type_ptr.hpp>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

#include <render_util/gl_binding/gl_functions.h>


using namespace render_util::gl_binding;
using std::cout;
using std::endl;


/*
<p>The rest of this file is organized in 3 parts:
<ul>
<li>the <a href="#shaders">first part</a> defines the shaders used to precompute
the atmospheric textures,</li>
<li>the <a href="#utilities">second part</a> provides utility classes and
functions used to compile shaders, create textures, draw quads, etc,</li>
<li>the <a href="#implementation">third part</a> provides the actual
implementation of the <code>Model</code> class, using the above tools.</li>
</ul>

<h3 id="shaders">Shader definitions</h3>

<p>In order to precompute a texture we attach it to a framebuffer object (FBO)
and we render a full quad in this FBO. For this we need a basic vertex shader:
*/

namespace atmosphere {

namespace {



struct TexUnits
{
  enum : unsigned int
  {
    TRANSMITTANCE = 0,
    IRRADIANCE,
    SINGLE_RAYLEIGH_SCATTERING,
    SINGLE_MIE_SCATTERING,
    MULTIPLE_SCATTERING,
    SCATTERING_DENSITY
  };
};


void dumpScatteringTexture(unsigned id, std::string name_prefix)
{
  FORCE_CHECK_GL_ERROR();
  auto slice_size = SCATTERING_TEXTURE_WIDTH * SCATTERING_TEXTURE_HEIGHT * 3;
  auto size = slice_size * SCATTERING_TEXTURE_DEPTH;
  std::vector<unsigned char> data(size);
  gl::GetTextureImage(id, 0, GL_RGB, GL_UNSIGNED_BYTE,
                      data.size(), data.data());
  FORCE_CHECK_GL_ERROR();

  for (int i = 0; i < SCATTERING_TEXTURE_DEPTH; i++)
  {
    std::string name = name_prefix;
    name += std::to_string(i) + ".png";
    render_util::saveImage(name, 3,
                           SCATTERING_TEXTURE_WIDTH,
                           SCATTERING_TEXTURE_HEIGHT,
                           data.data() + i * slice_size, slice_size,
                           render_util::ImageType::PNG);
  }
}


glm::mat3 make_glm_mat3(const std::array<float, 9> &in)
{
  return glm::transpose(glm::make_mat3(in.data()));
}


void bindTexture2D(unsigned int unit, unsigned int texture)
{
  gl::ActiveTexture(GL_TEXTURE0 + unit);
  gl::BindTexture(GL_TEXTURE_2D, texture);
}

void bindTexture3D(unsigned int unit, unsigned int texture)
{
  gl::ActiveTexture(GL_TEXTURE0 + unit);
  gl::BindTexture(GL_TEXTURE_3D, texture);
}


void useProgram(render_util::ShaderProgramPtr program, bool assert_uniforms_are_set = true)
{
  gl::UseProgram(program->getId());
  if (assert_uniforms_are_set)
    program->assertUniformsAreSet();
}


/*<h3 id="utilities">Utility classes and functions</h3>


/*
<p>We also need functions to allocate the precomputed textures on GPU:
*/

GLuint NewTexture2d(int width, int height) {
  GLuint texture;
  gl::GenTextures(1, &texture);
  gl::ActiveTexture(GL_TEXTURE0);
  gl::BindTexture(GL_TEXTURE_2D, texture);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl::BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  // 16F precision for the transmittance gives artifacts.
  gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0,
      GL_RGBA, GL_FLOAT, NULL);
  gl::BindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

GLuint NewTexture3d(int width, int height, int depth, GLenum format,
    bool half_precision) {
  GLuint texture;
  gl::GenTextures(1, &texture);
  gl::ActiveTexture(GL_TEXTURE0);
  gl::BindTexture(GL_TEXTURE_3D, texture);
  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  gl::BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  GLenum internal_format = format == GL_RGBA ?
      (half_precision ? GL_RGBA16F : GL_RGBA32F) :
      (half_precision ? GL_RGB16F : GL_RGB32F);
  gl::TexImage3D(GL_TEXTURE_3D, 0, internal_format, width, height, depth, 0,
      format, GL_FLOAT, NULL);
  gl::BindTexture(GL_TEXTURE_3D, 0);
  return texture;
}

/*
<p>a function to test whether the RGB format is a supported renderbuffer color
format (the OpenGL 3.3 Core Profile specification requires support for the RGBA
formats, but not for the RGB ones):
*/

bool IsFramebufferRgbFormatSupported(bool half_precision) {
  GLuint test_fbo = 0;
  gl::GenFramebuffers(1, &test_fbo);
  gl::BindFramebuffer(GL_FRAMEBUFFER, test_fbo);
  GLuint test_texture = 0;
  gl::GenTextures(1, &test_texture);
  gl::BindTexture(GL_TEXTURE_2D, test_texture);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl::TexImage2D(GL_TEXTURE_2D, 0, half_precision ? GL_RGB16F : GL_RGB32F,
               1, 1, 0, GL_RGB, GL_FLOAT, NULL);
  gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, test_texture, 0);
  bool rgb_format_supported =
      gl::CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  gl::DeleteTextures(1, &test_texture);
  gl::DeleteFramebuffers(1, &test_fbo);
  return rgb_format_supported;
}

/*
<p>and a function to draw a full screen quad in an offscreen framebuffer (with
blending separately enabled or disabled for each color attachment):
*/

void DrawQuad(const std::vector<bool>& enable_blend, GLuint quad_vao) {
  for (unsigned int i = 0; i < enable_blend.size(); ++i) {
    if (enable_blend[i]) {
      gl::Enablei(GL_BLEND, i);
    }
  }

  gl::BindVertexArray(quad_vao);
  gl::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  gl::BindVertexArray(0);

  for (unsigned int i = 0; i < enable_blend.size(); ++i) {
    gl::Disablei(GL_BLEND, i);
  }
}

/*
<p>Finally, we need a utility function to compute the value of the conversion
constants *<code>_RADIANCE_TO_LUMINANCE</code>, used above to convert the
spectral results into luminance values. These are the constants k_r, k_g, k_b
described in Section 14.3 of <a href="https://arxiv.org/pdf/1612.04336.pdf">A
Qualitative and Quantitative Evaluation of 8 Clear Sky Models</a>.

<p>Computing their value requires an integral of a function times a CIE color
matching function. Thus, we first need functions to interpolate an arbitrary
function (specified by some samples), and a CIE color matching function
(specified by tabulated values), at an arbitrary wavelength. This is the purpose
of the following two functions:
*/

constexpr int kLambdaMin = 360;
constexpr int kLambdaMax = 830;

double CieColorMatchingFunctionTableValue(double wavelength, int column) {
  if (wavelength <= kLambdaMin || wavelength >= kLambdaMax) {
    return 0.0;
  }
  double u = (wavelength - kLambdaMin) / 5.0;
  int row = static_cast<int>(std::floor(u));
  assert(row >= 0 && row + 1 < 95);
  assert(CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row] <= wavelength &&
         CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1)] >= wavelength);
  u -= row;
  return CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * row + column] * (1.0 - u) +
      CIE_2_DEG_COLOR_MATCHING_FUNCTIONS[4 * (row + 1) + column] * u;
}

double Interpolate(
    const std::vector<double>& wavelengths,
    const std::vector<double>& wavelength_function,
    double wavelength) {
  assert(wavelength_function.size() == wavelengths.size());
  if (wavelength < wavelengths[0]) {
    return wavelength_function[0];
  }
  for (unsigned int i = 0; i < wavelengths.size() - 1; ++i) {
    if (wavelength < wavelengths[i + 1]) {
      double u =
          (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
      return
          wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
    }
  }
  return wavelength_function[wavelength_function.size() - 1];
}

/*
<p>We can then implement a utility function to compute the "spectral radiance to
luminance" conversion constants (see Section 14.3 in <a
href="https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
Evaluation of 8 Clear Sky Models</a> for their definitions):
*/

// The returned constants are in lumen.nm / watt.
void ComputeSpectralRadianceToLuminanceFactors(
    glm::dvec3 rgb_lambdas,
    const std::vector<double>& wavelengths,
    const std::vector<double>& solar_irradiance,
    double lambda_power, double* k_r, double* k_g, double* k_b) {
  *k_r = 0.0;
  *k_g = 0.0;
  *k_b = 0.0;
  double solar_r = Interpolate(wavelengths, solar_irradiance, rgb_lambdas.r);
  double solar_g = Interpolate(wavelengths, solar_irradiance, rgb_lambdas.g);
  double solar_b = Interpolate(wavelengths, solar_irradiance, rgb_lambdas.b);
  int dlambda = 1;
  for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
    double x_bar = CieColorMatchingFunctionTableValue(lambda, 1);
    double y_bar = CieColorMatchingFunctionTableValue(lambda, 2);
    double z_bar = CieColorMatchingFunctionTableValue(lambda, 3);
    const double* xyz2srgb = XYZ_TO_SRGB;
    double r_bar =
        xyz2srgb[0] * x_bar + xyz2srgb[1] * y_bar + xyz2srgb[2] * z_bar;
    double g_bar =
        xyz2srgb[3] * x_bar + xyz2srgb[4] * y_bar + xyz2srgb[5] * z_bar;
    double b_bar =
        xyz2srgb[6] * x_bar + xyz2srgb[7] * y_bar + xyz2srgb[8] * z_bar;
    double irradiance = Interpolate(wavelengths, solar_irradiance, lambda);
    *k_r += r_bar * irradiance / solar_r *
        pow(lambda / rgb_lambdas.r, lambda_power);
    *k_g += g_bar * irradiance / solar_g *
        pow(lambda / rgb_lambdas.g, lambda_power);
    *k_b += b_bar * irradiance / solar_b *
        pow(lambda / rgb_lambdas.b, lambda_power);
  }
  *k_r *= MAX_LUMINOUS_EFFICACY * dlambda;
  *k_g *= MAX_LUMINOUS_EFFICACY * dlambda;
  *k_b *= MAX_LUMINOUS_EFFICACY * dlambda;
}

}  // anonymous namespace

/*<h3 id="implementation">Model implementation</h3>

<p>Using the above utility functions and classes, we can now implement the
constructor of the <code>Model</code> class. This constructor generates a piece
of GLSL code that defines an <code>ATMOSPHERE</code> constant containing the
atmosphere parameters (we use constants instead of uniforms to enable constant
folding and propagation optimizations in the GLSL compiler), concatenated with
<a href="functions.glsl.html">functions.glsl</a>, and with
<code>kAtmosphereShader</code>, to get the shader exposed by our API in
<code>GetShader</code>. It also allocates the precomputed textures (but does not
initialize them), as well as a vertex buffer object to render a full screen quad
(used to render into the precomputed textures).
*/

Model::Model(
    glm::dvec3 rgb_lambdas,
    const std::vector<double>& wavelengths,
    const std::vector<double>& solar_irradiance,
    const double sun_angular_radius,
    double bottom_radius,
    double top_radius,
    const std::vector<DensityProfileLayer>& rayleigh_density,
    const std::vector<double>& rayleigh_scattering,
    const std::vector<DensityProfileLayer>& mie_density,
    const std::vector<double>& mie_scattering,
    const std::vector<double>& mie_extinction,
    double mie_phase_function_g,
    const std::vector<DensityProfileLayer>& absorption_density,
    const std::vector<double>& absorption_extinction,
    const std::vector<double>& ground_albedo,
    double max_sun_zenith_angle,
    double length_unit_in_meters,
    unsigned int num_precomputed_wavelengths,
    bool combine_scattering_textures,
    bool half_precision,
    std::string shader_dir,
    const render_util::TextureManager &tex_mgr) :
        rgb_lambdas(rgb_lambdas),
        num_precomputed_wavelengths_(num_precomputed_wavelengths),
        half_precision_(half_precision),
        rgb_format_supported_(IsFramebufferRgbFormatSupported(half_precision)),
        m_texture_manager(tex_mgr)
{
  m_shader_search_path.push_back(shader_dir + "/internal");
  m_shader_search_path.push_back(shader_dir);

  auto to_string = [wavelengths](const std::vector<double>& v,
      const vec3& lambdas, double scale) {
    double r = Interpolate(wavelengths, v, lambdas[0]) * scale;
    double g = Interpolate(wavelengths, v, lambdas[1]) * scale;
    double b = Interpolate(wavelengths, v, lambdas[2]) * scale;
    return "vec3(" + std::to_string(r) + "," + std::to_string(g) + "," +
        std::to_string(b) + ")";
  };
  auto density_layer =
      [length_unit_in_meters](const DensityProfileLayer& layer) {
        return "DensityProfileLayer(" +
            std::to_string(layer.width / length_unit_in_meters) + "," +
            std::to_string(layer.exp_term) + "," +
            std::to_string(layer.exp_scale * length_unit_in_meters) + "," +
            std::to_string(layer.linear_term * length_unit_in_meters) + "," +
            std::to_string(layer.constant_term) + ")";
      };
  auto density_profile =
      [density_layer](std::vector<DensityProfileLayer> layers) {
        constexpr int kLayerCount = 2;
        while (layers.size() < kLayerCount) {
          layers.insert(layers.begin(), DensityProfileLayer());
        }
        std::string result = "DensityProfile(DensityProfileLayer[" +
            std::to_string(kLayerCount) + "](";
        for (int i = 0; i < kLayerCount; ++i) {
          result += density_layer(layers[i]);
          result += i < kLayerCount - 1 ? "," : "))";
        }
        return result;
      };

  // Compute the values for the SKY_RADIANCE_TO_LUMINANCE constant. In theory
  // this should be 1 in precomputed illuminance mode (because the precomputed
  // textures already contain illuminance values). In practice, however, storing
  // true illuminance values in half precision textures yields artefacts
  // (because the values are too large), so we store illuminance values divided
  // by MAX_LUMINOUS_EFFICACY instead. This is why, in precomputed illuminance
  // mode, we set SKY_RADIANCE_TO_LUMINANCE to MAX_LUMINOUS_EFFICACY.
  bool precompute_illuminance = num_precomputed_wavelengths > 3;
  double sky_k_r, sky_k_g, sky_k_b;
  if (precompute_illuminance) {
    sky_k_r = sky_k_g = sky_k_b = MAX_LUMINOUS_EFFICACY;
  } else {
    ComputeSpectralRadianceToLuminanceFactors(rgb_lambdas, wavelengths, solar_irradiance,
        -3 /* lambda_power */, &sky_k_r, &sky_k_g, &sky_k_b);
  }
  // Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
  double sun_k_r, sun_k_g, sun_k_b;
  ComputeSpectralRadianceToLuminanceFactors(rgb_lambdas, wavelengths, solar_irradiance,
      0 /* lambda_power */, &sun_k_r, &sun_k_g, &sun_k_b);


  // Allocate the precomputed textures, but don't precompute them yet.
  transmittance_texture_ = NewTexture2d(
      TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
  scattering_texture_ = NewTexture3d(
      SCATTERING_TEXTURE_WIDTH,
      SCATTERING_TEXTURE_HEIGHT,
      SCATTERING_TEXTURE_DEPTH,
      combine_scattering_textures || !rgb_format_supported_ ? GL_RGBA : GL_RGB,
      half_precision);
  if (combine_scattering_textures) {
    optional_single_mie_scattering_texture_ = 0;
  } else {
    optional_single_mie_scattering_texture_ = NewTexture3d(
        SCATTERING_TEXTURE_WIDTH,
        SCATTERING_TEXTURE_HEIGHT,
        SCATTERING_TEXTURE_DEPTH,
        rgb_format_supported_ ? GL_RGB : GL_RGBA,
        half_precision);
  }
  irradiance_texture_ = NewTexture2d(
      IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);

//   unsigned int prev_vertex_array_binding = 0;
//   gl::GetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&prev_vertex_array_binding);
//   FORCE_CHECK_GL_ERROR();

  // Create a full screen quad vertex array and vertex buffer objects.
  gl::GenVertexArrays(1, &full_screen_quad_vao_);
  gl::BindVertexArray(full_screen_quad_vao_);
  gl::GenBuffers(1, &full_screen_quad_vbo_);
  gl::BindBuffer(GL_ARRAY_BUFFER, full_screen_quad_vbo_);
  const GLfloat vertices[] = {
    -1.0, -1.0,
    +1.0, -1.0,
    -1.0, +1.0,
    +1.0, +1.0,
  };
  constexpr int kCoordsPerVertex = 2;
  gl::BufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
  constexpr GLuint kAttribIndex = 0;
  gl::VertexAttribPointer(kAttribIndex, kCoordsPerVertex, GL_FLOAT, false, 0, 0);
  gl::EnableVertexAttribArray(kAttribIndex);
  gl::BindVertexArray(0);
  gl::BindBuffer(GL_ARRAY_BUFFER, 0);

//   gl::BindVertexArray(prev_vertex_array_binding);

  auto sky_k = std::string("vec3(") +
          std::to_string(sky_k_r) + "," +
          std::to_string(sky_k_g) + "," +
          std::to_string(sky_k_b) + ")";

  auto sun_k = std::string("vec3(") +
          std::to_string(sun_k_r) + "," +
          std::to_string(sun_k_g) + "," +
          std::to_string(sun_k_b) + ")";

  m_shader_parameter_factory = [=](const vec3& lambdas)
  {
    render_util::ShaderParameters params;

    params.set("sky_k", sky_k);
    params.set("sun_k", sun_k);

    params.set("precompute_illuminance", precompute_illuminance);
    params.set("solar_irradiance", to_string(solar_irradiance, lambdas, 1.0));
    params.set("bottom_radius", std::to_string(bottom_radius / length_unit_in_meters));
    params.set("top_radius", std::to_string(top_radius / length_unit_in_meters));
    params.set("rayleigh_density", density_profile(rayleigh_density));
    params.set("rayleigh_scattering",
                        to_string(rayleigh_scattering, lambdas, length_unit_in_meters));
    params.set("mie_density", density_profile(mie_density));
    params.set("mie_scattering", to_string(mie_scattering, lambdas, length_unit_in_meters));
    params.set("mie_extinction", to_string(mie_extinction, lambdas, length_unit_in_meters));
    params.set("mie_phase_function_g", std::to_string(mie_phase_function_g));
    params.set("absorption_density", density_profile(absorption_density));
    params.set("absorption_extinction",
                        to_string(absorption_extinction, lambdas, length_unit_in_meters));
    params.set("ground_albedo", to_string(ground_albedo, lambdas, 1.0));
    params.set("mu_s_min", std::to_string(cos(max_sun_zenith_angle)));

    params.set("transmittance_texture_width", TRANSMITTANCE_TEXTURE_WIDTH);
    params.set("transmittance_texture_height", TRANSMITTANCE_TEXTURE_HEIGHT);
    params.set("scattering_texture_r_size", SCATTERING_TEXTURE_R_SIZE);
    params.set("scattering_texture_mu_size", SCATTERING_TEXTURE_MU_SIZE);
    params.set("scattering_texture_mu_s_size", SCATTERING_TEXTURE_MU_S_SIZE);
    params.set("scattering_texture_nu_size", SCATTERING_TEXTURE_NU_SIZE);
    params.set("irradiance_texture_width", IRRADIANCE_TEXTURE_WIDTH);
    params.set("irradiance_texture_height", IRRADIANCE_TEXTURE_HEIGHT);

    params.set("combine_scattering_textures", combine_scattering_textures);

    params.set("sun_angular_radius", sun_angular_radius);

    return params;
  };

  vec3 lambdas = {rgb_lambdas.r, rgb_lambdas.g, rgb_lambdas.b};

  m_shader_params = m_shader_parameter_factory(lambdas);
}

/*
<p>The destructor is trivial:
*/

Model::~Model() {
  gl::DeleteBuffers(1, &full_screen_quad_vbo_);
  gl::DeleteVertexArrays(1, &full_screen_quad_vao_);
  gl::DeleteTextures(1, &transmittance_texture_);
  gl::DeleteTextures(1, &scattering_texture_);
  if (optional_single_mie_scattering_texture_ != 0) {
    gl::DeleteTextures(1, &optional_single_mie_scattering_texture_);
  }
  gl::DeleteTextures(1, &irradiance_texture_);
}

/*
<p>The Init method precomputes the atmosphere textures. It first allocates the
temporary resources it needs, then calls <code>Precompute</code> to do the
actual precomputations, and finally destroys the temporary resources.

<p>Note that there are two precomputation modes here, depending on whether we
want to store precomputed irradiance or illuminance values:
<ul>
  <li>In precomputed irradiance mode, we simply need to call
  <code>Precompute</code> with the 3 wavelengths for which we want to precompute
  irradiance, namely <code>kLambdaR</code>, <code>kLambdaG</code>,
  <code>kLambdaB</code> (with the identity matrix for
  <code>luminance_from_radiance</code>, since we don't want any conversion from
  radiance to luminance)</li>
  <li>In precomputed illuminance mode, we need to precompute irradiance for
  <code>num_precomputed_wavelengths_</code>, and then integrate the results,
  multiplied with the 3 CIE xyz color matching functions and the XYZ to sRGB
  matrix to get sRGB illuminance values.
  <p>A naive solution would be to allocate temporary textures for the
  intermediate irradiance results, then perform the integration from irradiance
  to illuminance and store the result in the final precomputed texture. In
  pseudo-code (and assuming one wavelength per texture instead of 3):
  <pre>
    create n temporary irradiance textures
    for each wavelength lambda in the n wavelengths:
       precompute irradiance at lambda into one of the temporary textures
    initializes the final illuminance texture with zeros
    for each wavelength lambda in the n wavelengths:
      accumulate in the final illuminance texture the product of the
      precomputed irradiance at lambda (read from the temporary textures)
      with the value of the 3 sRGB color matching functions at lambda (i.e.
      the product of the XYZ to sRGB matrix with the CIE xyz color matching
      functions).
  </pre>
  <p>However, this be would waste GPU memory. Instead, we can avoid allocating
  temporary irradiance textures, by merging the two above loops:
  <pre>
    for each wavelength lambda in the n wavelengths:
      accumulate in the final illuminance texture (or, for the first
      iteration, set this texture to) the product of the precomputed
      irradiance at lambda (computed on the fly) with the value of the 3
      sRGB color matching functions at lambda.
  </pre>
  <p>This is the method we use below, with 3 wavelengths per iteration instead
  of 1, using <code>Precompute</code> to compute 3 irradiances values per
  iteration, and <code>luminance_from_radiance</code> to multiply 3 irradiances
  with the values of the 3 sRGB color matching functions at 3 different
  wavelengths (yielding a 3x3 matrix).</li>
</ul>

<p>This yields the following implementation:
*/

void Model::Init(unsigned int num_scattering_orders) {
  // The precomputations require temporary textures, in particular to store the
  // contribution of one scattering order, which is needed to compute the next
  // order of scattering (the final precomputed textures store the sum of all
  // the scattering orders). We allocate them here, and destroy them at the end
  // of this method.
  GLuint delta_irradiance_texture = NewTexture2d(
      IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
  GLuint delta_rayleigh_scattering_texture = NewTexture3d(
      SCATTERING_TEXTURE_WIDTH,
      SCATTERING_TEXTURE_HEIGHT,
      SCATTERING_TEXTURE_DEPTH,
      rgb_format_supported_ ? GL_RGB : GL_RGBA,
      half_precision_);
  GLuint delta_mie_scattering_texture = NewTexture3d(
      SCATTERING_TEXTURE_WIDTH,
      SCATTERING_TEXTURE_HEIGHT,
      SCATTERING_TEXTURE_DEPTH,
      rgb_format_supported_ ? GL_RGB : GL_RGBA,
      half_precision_);
  GLuint delta_scattering_density_texture = NewTexture3d(
      SCATTERING_TEXTURE_WIDTH,
      SCATTERING_TEXTURE_HEIGHT,
      SCATTERING_TEXTURE_DEPTH,
      rgb_format_supported_ ? GL_RGB : GL_RGBA,
      half_precision_);
  // delta_multiple_scattering_texture is only needed to compute scattering
  // order 3 or more, while delta_rayleigh_scattering_texture and
  // delta_mie_scattering_texture are only needed to compute double scattering.
  // Therefore, to save memory, we can store delta_rayleigh_scattering_texture
  // and delta_multiple_scattering_texture in the same GPU texture.
  GLuint delta_multiple_scattering_texture = delta_rayleigh_scattering_texture;

  // The precomputations also require a temporary framebuffer object, created
  // here (and destroyed at the end of this method).
  GLuint fbo;
  gl::GenFramebuffers(1, &fbo);
  gl::BindFramebuffer(GL_FRAMEBUFFER, fbo);

  // The actual precomputations depend on whether we want to store precomputed
  // irradiance or illuminance values.
  if (num_precomputed_wavelengths_ <= 3) {
    vec3 lambdas{rgb_lambdas.r, rgb_lambdas.g, rgb_lambdas.b};
    mat3 luminance_from_radiance{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    Precompute(fbo, delta_irradiance_texture, delta_rayleigh_scattering_texture,
        delta_mie_scattering_texture, delta_scattering_density_texture,
        delta_multiple_scattering_texture, lambdas, luminance_from_radiance,
        false /* blend */, num_scattering_orders);
  } else {
    constexpr double kLambdaMin = 360.0;
    constexpr double kLambdaMax = 830.0;
    int num_iterations = (num_precomputed_wavelengths_ + 2) / 3;
    double dlambda = (kLambdaMax - kLambdaMin) / (3 * num_iterations);
    for (int i = 0; i < num_iterations; ++i) {
      vec3 lambdas{
        kLambdaMin + (3 * i + 0.5) * dlambda,
        kLambdaMin + (3 * i + 1.5) * dlambda,
        kLambdaMin + (3 * i + 2.5) * dlambda
      };
      auto coeff = [dlambda](double lambda, int component) {
        // Note that we don't include MAX_LUMINOUS_EFFICACY here, to avoid
        // artefacts due to too large values when using half precision on GPU.
        // We add this term back in kAtmosphereShader, via
        // SKY_SPECTRAL_RADIANCE_TO_LUMINANCE (see also the comments in the
        // Model constructor).
        double x = CieColorMatchingFunctionTableValue(lambda, 1);
        double y = CieColorMatchingFunctionTableValue(lambda, 2);
        double z = CieColorMatchingFunctionTableValue(lambda, 3);
        return static_cast<float>((
            XYZ_TO_SRGB[component * 3] * x +
            XYZ_TO_SRGB[component * 3 + 1] * y +
            XYZ_TO_SRGB[component * 3 + 2] * z) * dlambda);
      };
      mat3 luminance_from_radiance{
        coeff(lambdas[0], 0), coeff(lambdas[1], 0), coeff(lambdas[2], 0),
        coeff(lambdas[0], 1), coeff(lambdas[1], 1), coeff(lambdas[2], 1),
        coeff(lambdas[0], 2), coeff(lambdas[1], 2), coeff(lambdas[2], 2)
      };
      Precompute(fbo, delta_irradiance_texture,
          delta_rayleigh_scattering_texture, delta_mie_scattering_texture,
          delta_scattering_density_texture, delta_multiple_scattering_texture,
          lambdas, luminance_from_radiance, i > 0 /* blend */,
          num_scattering_orders);
    }

    // After the above iterations, the transmittance texture contains the
    // transmittance for the 3 wavelengths used at the last iteration. But we
    // want the transmittance at kLambdaR, kLambdaG, kLambdaB instead, so we
    // must recompute it here for these 3 wavelengths:
    {
      auto program = createShaderProgram("compute_transmittance", m_shader_params);

      gl::FramebufferTexture(
          GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, transmittance_texture_, 0);
      gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
      gl::Viewport(0, 0, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
      useProgram(program);
      program->assertUniformsAreSet();
      DrawQuad({}, full_screen_quad_vao_);
    }
  }

  // Delete the temporary resources allocated at the begining of this method.
  gl::UseProgram(0);
  gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
  gl::DeleteFramebuffers(1, &fbo);
  gl::DeleteTextures(1, &delta_scattering_density_texture);
  gl::DeleteTextures(1, &delta_mie_scattering_texture);
  gl::DeleteTextures(1, &delta_rayleigh_scattering_texture);
  gl::DeleteTextures(1, &delta_irradiance_texture);
  assert(gl::GetError() == 0);

  gl::DrawBuffer(GL_BACK);
  FORCE_CHECK_GL_ERROR();

//   dumpScatteringTexture(scattering_texture_, "scattering");
//   dumpScatteringTexture(optional_single_mie_scattering_texture_, "single_mie_scattering");
}

/*
<p>The <code>SetProgramUniforms</code> method is straightforward: it simply
binds the precomputed textures to the specified texture units, and then sets
the corresponding uniforms in the user provided program to the index of these
texture units.
*/

void Model::SetProgramUniforms(
    render_util::ShaderProgramPtr program,
    GLuint transmittance_texture_unit,
    GLuint scattering_texture_unit,
    GLuint irradiance_texture_unit,
    GLuint single_mie_scattering_texture_unit) const
{
  GLenum active_unit_save;
  gl::GetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint*>(&active_unit_save));

  gl::ActiveTexture(GL_TEXTURE0 + transmittance_texture_unit);
  gl::BindTexture(GL_TEXTURE_2D, transmittance_texture_);
  program->setUniformi("transmittance_texture", transmittance_texture_unit);

  gl::ActiveTexture(GL_TEXTURE0 + scattering_texture_unit);
  gl::BindTexture(GL_TEXTURE_3D, scattering_texture_);
  program->setUniformi("scattering_texture", scattering_texture_unit);

  gl::ActiveTexture(GL_TEXTURE0 + irradiance_texture_unit);
  gl::BindTexture(GL_TEXTURE_2D, irradiance_texture_);
  program->setUniformi("irradiance_texture", irradiance_texture_unit);

  if (optional_single_mie_scattering_texture_ != 0) {
    gl::ActiveTexture(GL_TEXTURE0 + single_mie_scattering_texture_unit);
    gl::BindTexture(GL_TEXTURE_3D, optional_single_mie_scattering_texture_);
    program->setUniformi("single_mie_scattering_texture", single_mie_scattering_texture_unit);
  }

  gl::ActiveTexture(active_unit_save);
}

/*
<p>The utility method <code>ConvertSpectrumToLinearSrgb</code> is implemented
with a simple numerical integration of the given function, times the CIE color
matching funtions (with an integration step of 1nm), followed by a matrix
multiplication:
*/

void Model::ConvertSpectrumToLinearSrgb(
    const std::vector<double>& wavelengths,
    const std::vector<double>& spectrum,
    double* r, double* g, double* b) {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  const int dlambda = 1;
  for (int lambda = kLambdaMin; lambda < kLambdaMax; lambda += dlambda) {
    double value = Interpolate(wavelengths, spectrum, lambda);
    x += CieColorMatchingFunctionTableValue(lambda, 1) * value;
    y += CieColorMatchingFunctionTableValue(lambda, 2) * value;
    z += CieColorMatchingFunctionTableValue(lambda, 3) * value;
  }
  *r = MAX_LUMINOUS_EFFICACY *
      (XYZ_TO_SRGB[0] * x + XYZ_TO_SRGB[1] * y + XYZ_TO_SRGB[2] * z) * dlambda;
  *g = MAX_LUMINOUS_EFFICACY *
      (XYZ_TO_SRGB[3] * x + XYZ_TO_SRGB[4] * y + XYZ_TO_SRGB[5] * z) * dlambda;
  *b = MAX_LUMINOUS_EFFICACY *
      (XYZ_TO_SRGB[6] * x + XYZ_TO_SRGB[7] * y + XYZ_TO_SRGB[8] * z) * dlambda;
}

/*
<p>Finally, we provide the actual implementation of the precomputation algorithm
described in Algorithm 4.1 of
<a href="https://hal.inria.fr/inria-00288758/en">our paper</a>. Each step is
explained by the inline comments below.
*/
void Model::Precompute(
    GLuint fbo,
    GLuint delta_irradiance_texture,
    GLuint delta_rayleigh_scattering_texture,
    GLuint delta_mie_scattering_texture,
    GLuint delta_scattering_density_texture,
    GLuint delta_multiple_scattering_texture,
    const vec3& lambdas,
    const mat3& luminance_from_radiance,
    bool blend,
    unsigned int num_scattering_orders)
{
  FORCE_CHECK_GL_ERROR();

  auto shader_params = m_shader_parameter_factory(lambdas);

  auto createProgram = [this, shader_params] (std::string name)
  {
    return createShaderProgram(name, shader_params);
  };

  auto compute_transmittance = createProgram("compute_transmittance");
  auto compute_direct_irradiance = createProgram("compute_direct_irradiance");
  auto compute_single_scattering = createProgram("compute_single_scattering");
  auto compute_scattering_density = createProgram("compute_scattering_density");
  auto compute_indirect_irradiance = createProgram("compute_indirect_irradiance");
  auto compute_multiple_scattering = createProgram("compute_multiple_scattering");

  bindTexture2D(TexUnits::TRANSMITTANCE, transmittance_texture_);
  bindTexture3D(TexUnits::SINGLE_RAYLEIGH_SCATTERING, delta_rayleigh_scattering_texture);
  bindTexture3D(TexUnits::SINGLE_MIE_SCATTERING, delta_mie_scattering_texture);
  bindTexture3D(TexUnits::MULTIPLE_SCATTERING, delta_multiple_scattering_texture);
  bindTexture2D(TexUnits::IRRADIANCE, delta_irradiance_texture);
  bindTexture3D(TexUnits::SCATTERING_DENSITY, delta_scattering_density_texture);

  FORCE_CHECK_GL_ERROR();

  const GLuint kDrawBuffers[4] = {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2,
    GL_COLOR_ATTACHMENT3
  };
  gl::BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
  gl::BlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

  // Compute the transmittance, and store it in transmittance_texture_.
  gl::FramebufferTexture(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, transmittance_texture_, 0);
  gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
  gl::Viewport(0, 0, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
  useProgram(compute_transmittance);
  DrawQuad({}, full_screen_quad_vao_);

  // Compute the direct irradiance, store it in delta_irradiance_texture and,
  // depending on 'blend', either initialize irradiance_texture_ with zeros or
  // leave it unchanged (we don't want the direct irradiance in
  // irradiance_texture_, but only the irradiance from the sky).
  gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      delta_irradiance_texture, 0);
  gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
      irradiance_texture_, 0);
  gl::DrawBuffers(2, kDrawBuffers);
  gl::Viewport(0, 0, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
  useProgram(compute_direct_irradiance);
  DrawQuad({false, blend}, full_screen_quad_vao_);

  // Compute the rayleigh and mie single scattering, store them in
  // delta_rayleigh_scattering_texture and delta_mie_scattering_texture, and
  // either store them or accumulate them in scattering_texture_ and
  // optional_single_mie_scattering_texture_.
  gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      delta_rayleigh_scattering_texture, 0);
  gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
      delta_mie_scattering_texture, 0);
  gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
      scattering_texture_, 0);
  if (optional_single_mie_scattering_texture_ != 0) {
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
        optional_single_mie_scattering_texture_, 0);
    gl::DrawBuffers(4, kDrawBuffers);
  } else {
    gl::DrawBuffers(3, kDrawBuffers);
  }
  gl::Viewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
  compute_single_scattering->setUniform("luminance_from_radiance",
                                        make_glm_mat3(luminance_from_radiance));
  useProgram(compute_single_scattering, false);
  for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer) {
    compute_single_scattering->setUniformi("layer", layer);
    compute_single_scattering->assertUniformsAreSet();
    DrawQuad({false, false, blend, blend}, full_screen_quad_vao_);
  }

  // Compute the 2nd, 3rd and 4th order of scattering, in sequence.
  for (unsigned int scattering_order = 2;
       scattering_order <= num_scattering_orders;
       ++scattering_order) {
    // Compute the scattering density, and store it in
    // delta_scattering_density_texture.
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        delta_scattering_density_texture, 0);
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, 0, 0);
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, 0, 0);
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, 0, 0);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::Viewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
    compute_scattering_density->setUniformi("scattering_order", scattering_order);
    useProgram(compute_scattering_density, false);
    for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer) {
      compute_scattering_density->setUniformi("layer", layer);
      compute_scattering_density->assertUniformsAreSet();
      DrawQuad({}, full_screen_quad_vao_);
    }

    // Compute the indirect irradiance, store it in delta_irradiance_texture and
    // accumulate it in irradiance_texture_.
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        delta_irradiance_texture, 0);
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
        irradiance_texture_, 0);
    gl::DrawBuffers(2, kDrawBuffers);
    gl::Viewport(0, 0, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
    compute_indirect_irradiance->setUniform("luminance_from_radiance",
                                            make_glm_mat3(luminance_from_radiance));
    compute_indirect_irradiance->setUniformi("scattering_order", scattering_order - 1);
    useProgram(compute_indirect_irradiance);
    DrawQuad({false, true}, full_screen_quad_vao_);

    // Compute the multiple scattering, store it in
    // delta_multiple_scattering_texture, and accumulate it in
    // scattering_texture_.
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        delta_multiple_scattering_texture, 0);
    gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
        scattering_texture_, 0);
    gl::DrawBuffers(2, kDrawBuffers);
    gl::Viewport(0, 0, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT);
    compute_multiple_scattering->setUniform("luminance_from_radiance",
                                            make_glm_mat3(luminance_from_radiance));
    useProgram(compute_multiple_scattering, false);
    for (unsigned int layer = 0; layer < SCATTERING_TEXTURE_DEPTH; ++layer) {
      compute_multiple_scattering->setUniformi("layer", layer);
      compute_multiple_scattering->assertUniformsAreSet();
      DrawQuad({false, true}, full_screen_quad_vao_);
    }
  }

  gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, 0, 0);
  gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, 0, 0);
  gl::FramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, 0, 0);
}


render_util::ShaderParameters Model::getShaderParameters()
{
  return m_shader_params;
}


render_util::ShaderProgramPtr Model::createShaderProgram(std::string name,
                                                         const render_util::ShaderParameters &params)
{
  auto program = render_util::createShaderProgram(name, m_texture_manager,
                                                  m_shader_search_path, {}, params);

  program->setUniformi("transmittance_texture", TexUnits::TRANSMITTANCE);
  program->setUniformi("irradiance_texture", TexUnits::IRRADIANCE);
  program->setUniformi("single_rayleigh_scattering_texture", TexUnits::SINGLE_RAYLEIGH_SCATTERING);
  program->setUniformi("single_mie_scattering_texture", TexUnits::SINGLE_MIE_SCATTERING);
  program->setUniformi("multiple_scattering_texture", TexUnits::MULTIPLE_SCATTERING);
  program->setUniformi("scattering_density_texture", TexUnits::SCATTERING_DENSITY);

  return program;
}


}  // namespace atmosphere
