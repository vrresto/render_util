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

#ifndef RENDER_UTIL_AERIAL_PERSPECTIVE_H
#define RENDER_UTIL_AERIAL_PERSPECTIVE_H

#include <render_util/camera.h>
#include <render_util/texture_manager.h>
#include <render_util/shader.h>

#include <glm/glm.hpp>
#include <functional>

namespace render_util::aerial_perspective
{


struct FrustumTextureFrame
{
  const unsigned int texunit = 0;
  glm::vec3 sample_offset = glm::vec3(0);
  TexturePtr texture;
  Camera camera;
};


class AerialPerspective
{
public:
  using UpdateUniformsFunc = std::function<void(ShaderProgramPtr)>;

  AerialPerspective(UpdateUniformsFunc update_scene_uniforms,
                    ShaderSearchPath shader_search_path,
                    const TextureManager &txmgr);

  ShaderParameters getShaderParameters() { return m_shader_params; }

  void bindTextures(TextureManager &txmgr);
  void computeStep(const Camera &camera, const TextureManager &txmgr);
  void updateUniforms(ShaderProgramPtr program, const TextureManager &txmgr);

private:
  std::vector<FrustumTextureFrame> m_frustum_texture_frames;
  int m_current_frustum_texture_frame = 0;
  ShaderProgramPtr m_compute_program;
  ShaderParameters m_shader_params;
  UpdateUniformsFunc m_update_scene_uniforms;

  bool are_textures_bound = false;
};


}

#endif
