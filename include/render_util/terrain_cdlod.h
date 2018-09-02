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

#ifndef RENDER_UTIL_TERRAIN_CDLOD_H
#define RENDER_UTIL_TERRAIN_CDLOD_H

#include <render_util/terrain_base.h>
#include <render_util/image.h>

namespace render_util
{
  class ElevationMap;

  class TerrainCDLOD : public TerrainBase
  {
    struct Private;
    Private *p = 0;

  public:
    TerrainCDLOD();
    ~TerrainCDLOD() override;

    const std::string &getName() override;
    void build(const ElevationMap *map) override;
    void draw(ShaderProgramPtr program) override;
    void update(const Camera &camera) override;
    void setTextureManager(TextureManager*) override;
    void setDrawDistance(float dist) override;
  };

}

#endif
