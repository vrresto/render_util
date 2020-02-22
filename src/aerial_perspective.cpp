/**
 *    Rendering utilities
 *    Copyright (C) 2020 Jan Lepper
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

#include <render_util/aerial_perspective.h>
#include <render_util/texunits.h>
#include <render_util/shader_util.h>
#include <render_util/globals.h>
#include <render_util/gl_binding/gl_functions.h>

using namespace render_util;
using namespace render_util::aerial_perspective;
using namespace render_util::gl_binding;

namespace
{


constexpr int frustum_texture_scale = 1;
constexpr int frustum_texture_depth_scale = 1;

constexpr glm::ivec3 frustum_texture_res = glm::ivec3(160 * frustum_texture_scale,
                                                      90 * frustum_texture_scale,
                                                      128 * frustum_texture_depth_scale);


inline TexturePtr createFrustumTexture()
{
  auto texture = Texture::create(GL_TEXTURE_3D);

  TemporaryTextureBinding binding(texture);

  int tex_w = frustum_texture_res.x, tex_h = frustum_texture_res.y;
  int tex_depth = frustum_texture_res.z;

  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  gl::TexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, tex_w, tex_h, tex_depth);

  return texture;
}


inline void createFrustumTextureFrames(std::vector<FrustumTextureFrame> &frames)
{
  int subdivisions = 2;
//   auto overall_offset = glm::vec3(-0.25);
  auto overall_offset = glm::vec3(0);

  for (int x = 0; x < subdivisions; x++)
  {
    for (int y = 0; y < subdivisions; y++)
    {
      for (int z = 0; z < subdivisions; z++)
      {
        auto texunit = TEXUNIT_AERIAL_PERSPECTIVE_0 + frames.size();
        assert(texunit <= TEXUNIT_AERIAL_PERSPECTIVE_7);

        auto offset = glm::vec3(x,y,z) / glm::vec3(subdivisions);
        offset += overall_offset;

        FrustumTextureFrame frame
        {
          .texunit = texunit,
          .sample_offset = offset,
          .texture = createFrustumTexture(),
          .camera = {},
        };

        frames.push_back(frame);
      }
    }
  }

  assert (frames.size() == 8);
}


inline void bindFrustumTextureFrames(const std::vector<FrustumTextureFrame> &frames,
                                     TextureManager &txmgr)
{
  for (auto &frame : frames)
  {
    txmgr.bind(frame.texunit, frame.texture);
  }
}


inline void setFrustumTextureCameraUniforms(render_util::ShaderProgramPtr program,
                              const std::string &prefix,
                              const render_util::Camera &camera)
{
  program->setUniform(prefix +  "ndc_xy_to_view", camera.getNDCToView());

  program->setUniform(prefix + "camera_pos", camera.getPos());
  program->setUniform(prefix + "projection_matrix", camera.getProjectionMatrixFar());
  program->setUniform(prefix + "world_to_view_matrix", camera.getWorld2ViewMatrix());
  program->setUniform(prefix + "view_to_world_rotation_matrix", camera.getViewToWorldRotation());

  program->setUniform<float>(prefix + "fov", camera.getFov());
  program->setUniform<float>(prefix + "z_near", camera.getZNear());
  program->setUniform<float>(prefix + "z_far", camera.getZFar());
}


} // namespace


namespace render_util::aerial_perspective
{


AerialPerspective::AerialPerspective(ShaderParameters shader_params,
            UpdateUniformsFunc update_scene_uniforms,
            ShaderSearchPath shader_search_path,
            const TextureManager &txmgr)
{
  m_update_scene_uniforms = update_scene_uniforms;

  createFrustumTextureFrames(m_frustum_texture_frames);
  assert(!m_frustum_texture_frames.empty());

  m_shader_params = shader_params;

  m_shader_params.set("frustum_texture_frames_num", m_frustum_texture_frames.size());

  m_compute_program =
    render_util::createShaderProgram("compute_aerial_perspective",
                                      txmgr,
                                      shader_search_path,
                                      {},
                                      m_shader_params);
}


void AerialPerspective::bindTextures(TextureManager &txmgr)
{
  bindFrustumTextureFrames(m_frustum_texture_frames, txmgr);
  are_textures_bound = true;
}


void AerialPerspective::computeStep(const render_util::Camera &camera, const TextureManager &txmgr)
{
  assert(are_textures_bound);

  m_current_frustum_texture_frame =
    (m_current_frustum_texture_frame + 1) % m_frustum_texture_frames.size();

  m_frustum_texture_frames[m_current_frustum_texture_frame].camera = camera;

  auto &frame = m_frustum_texture_frames[m_current_frustum_texture_frame];

  gl::MemoryBarrier(GL_ALL_BARRIER_BITS);

  bool layered = true;
  int layer = 0;
  gl::BindImageTexture(0, frame.texture->getID(),
                      0, layered, layer, GL_WRITE_ONLY, GL_RGBA32F);

  getCurrentGLContext()->setCurrentProgram(m_compute_program);

  m_update_scene_uniforms(m_compute_program);
  updateUniforms(m_compute_program, txmgr);
  m_compute_program->setUniform("texture_size", frustum_texture_res);
  m_compute_program->setUniformi("img_output", 0);
  m_compute_program->assertUniformsAreSet();

  gl::DispatchCompute(frustum_texture_res.x, frustum_texture_res.y, 1);

  // make sure writing to image has finished before read
  gl::MemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  gl::MemoryBarrier(GL_ALL_BARRIER_BITS);
  FORCE_CHECK_GL_ERROR();
}


void AerialPerspective::updateUniforms(render_util::ShaderProgramPtr program,
                                       const TextureManager &txmgr)
{
  assert(are_textures_bound);

  for (auto i = 0; i < m_frustum_texture_frames.size(); i++)
  {
    auto prefix = std::string("fustum_texture_frames[") + std::to_string(i) + "].";
    setFrustumTextureCameraUniforms(program, prefix, m_frustum_texture_frames[i].camera);
  }

  program->setUniform<unsigned int>("current_frustum_texture_frame",
                                    m_current_frustum_texture_frame);

  program->setUniform("frustum_texture_size", frustum_texture_res);

  for (auto i = 0; i < m_frustum_texture_frames.size(); i++)
  {
    auto prefix = std::string("fustum_texture_frames[") + std::to_string(i) + "].";
    auto real_texunit = txmgr.getTexUnitNum(m_frustum_texture_frames[i].texunit);

    program->setUniform(prefix + "sample_offset", m_frustum_texture_frames[i].sample_offset);
    program->setUniformi(prefix + "sampler", real_texunit);
  }
}


} // namespace render_util::aerial_perspective
