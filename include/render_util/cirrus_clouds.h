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

#ifndef RENDER_UTIL_CIRRUS_CLOUDS_H
#define RENDER_UTIL_CIRRUS_CLOUDS_H

#include <render_util/camera.h>
#include <render_util/shader.h>
#include <render_util/texture_manager.h>
#include <render_util/state.h>

#include <memory>

namespace render_util
{

class CirrusClouds
{
  struct Impl;
  std::unique_ptr<Impl> impl;

public:
  CirrusClouds(TextureManager &txmgr, const ShaderSearchPath&, const ShaderParameters&);
  ~CirrusClouds();

  ShaderProgramPtr getProgram();

  void draw(StateModifier &state, const Camera &camera);
};


}


#endif
