/**
 *    Rendering utilities
 *    Copyright (C) 2018  Jan Lepper
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

#include <render_util/water.h>
#include <render_util/image.h>
#include <render_util/image_loader.h>
#include <render_util/map_textures.h>
#include <render_util/texunits.h>

#include <vector>
#include <chrono>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "GL/gl.h"


namespace render_util
{


struct WaterAnimation::Private
{
};


WaterAnimation::WaterAnimation() : p(new Private) {}

WaterAnimation::~WaterAnimation()
{
  delete p;
}

void WaterAnimation::createTextures(MapTextures *map_textures,
                                    const std::vector<ImageRGBA::ConstPtr> &normal_maps,
                                    const std::vector<ImageGreyScale::ConstPtr> &foam_masks)
{
}

void WaterAnimation::update()
{
}

void WaterAnimation::updateUniforms(ShaderProgramPtr program)
{
}

bool WaterAnimation::isEmpty()
{
  abort();
}


} // namespace render_util
