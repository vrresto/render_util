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

#ifndef ENGINE_WATER_H
#define ENGINE_WATER_H

#include <engine/shader.h>

namespace engine
{

  class MapTextures;

  class WaterAnimation
  {
    struct Private;
    Private *p = 0;

  public:
    WaterAnimation();
    ~WaterAnimation();

    void createTextures(MapTextures *map_textures);

    void updateUniforms(ShaderProgramPtr program);

    int getCurrentStep();
    float getFrameDelta();
    void update();
  };

}

#endif
