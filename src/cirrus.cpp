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

#include "uv_dome.h"
#include "vao.h"

#include <render_util/render_util.h>
#include <render_util/cirrus_clouds.h>
#include <render_util/texture_manager.h>
#include <render_util/shader_util.h>
#include <render_util/texunits.h>
#include <render_util/texture_util.h>
#include <render_util/image_loader.h>
#include <render_util/geometry.h>
#include <render_util/globals.h>
#include <render_util/state.h>

#include <iostream>
#include <vector>
#include <map>
#include <glm/glm.hpp>

#include <render_util/gl_binding/gl_functions.h>

using namespace render_util::gl_binding;
using namespace render_util;
using std::cout;
using std::endl;


namespace render_util
{


struct CirrusClouds::Impl
{
  std::unique_ptr<VertexArrayObject> vao;

  render_util::TexturePtr texture;
  render_util::ShaderProgramPtr program;

  Impl(float height) : height(height) {}

  const float height = 0;
};


CirrusClouds::CirrusClouds(TextureManager &txmgr,
                           const ShaderSearchPath &shader_search_path,
                           const ShaderParameters &shader_params,
                           float height,
                           std::shared_ptr<const GenericImage> texture_image)
  : impl(std::make_unique<Impl>(height))
{
  auto mesh = generateUVDome(100, 100, 0.04 * util::PI);

  impl->vao = std::make_unique<VertexArrayObject>(mesh, true);

  impl->program = render_util::createShaderProgram("cirrus", txmgr, shader_search_path, {}, shader_params);
//   impl->program->setUniformi("sampler_generic_noise",
//                              txmgr.getTexUnitNum(TEXUNIT_GENERIC_NOISE));
  impl->program->setUniformi("sampler_cirrus",
                             txmgr.getTexUnitNum(TEXUNIT_CIRRUS));

  if (!texture_image)
    texture_image = render_util::loadImageFromFile<GenericImage>("cirrus.tga"); //FIXME

  assert(texture_image);

  assert(texture_image->numComponents() == 1 || texture_image->numComponents() == 4);

  if (texture_image->numComponents() == 1)
  {
    impl->texture = render_util::createTexture(texture_image);
  }
  else if (texture_image->numComponents() == 4)
  {
    auto alpha_channel = image::getChannel(texture_image, 3);
    impl->texture = render_util::createTexture(alpha_channel);
  }
  else
  {
    abort();
  }

  render_util::TextureParameters<int> p;
//   p.set(GL_TEXTURE_LOD_BIAS, 1.0);
  p.apply(impl->texture);

  txmgr.bind(TEXUNIT_CIRRUS, impl->texture);
}


CirrusClouds::~CirrusClouds() {}


void CirrusClouds::draw(const StateModifier &prev_state, const Camera &camera)
{
  StateModifier state(prev_state);

  state.setFrontFace(GL_CW);

  VertexArrayObjectBinding vao_binding(*impl->vao);
  IndexBufferBinding index_buffer_binding(*impl->vao);

  auto program = getCurrentGLContext()->getCurrentProgram();

  program->setUniform("cirrus_height", getHeight());
  program->setUniform("cirrus_layer_thickness", 100.f);

  program->setUniform("is_upper_side", false);
  state.setCullFace(GL_FRONT);
  gl::DrawElements(GL_TRIANGLES, impl->vao->getNumIndices(), GL_UNSIGNED_INT, nullptr);

  if (camera.getPos().z > getHeight())
  {
    program->setUniform("is_upper_side", true);
    state.setCullFace(GL_BACK);
    gl::DrawElements(GL_TRIANGLES, impl->vao->getNumIndices(), GL_UNSIGNED_INT, nullptr);
  }
}


ShaderProgramPtr CirrusClouds::getProgram()
{
  return impl->program;
}


float CirrusClouds::getHeight() const
{
  return impl->height;
}


}
