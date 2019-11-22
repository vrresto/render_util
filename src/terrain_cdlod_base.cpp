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

#include "terrain_cdlod_base.h"
#include <render_util/texture_util.h>
#include <render_util/gl_binding/gl_functions.h>
#include <log.h>

using namespace std;


namespace render_util
{


TexturePtr TerrainCDLODBase::createNormalMapTexture(render_util::ElevationMap::ConstPtr map,
                                                    int meters_per_grid)
{
  using namespace render_util;

  LOG_INFO<<"TerrainCDLOD: creating normal map ..."<<endl;
  auto normal_map = createNormalMapRGB(map, meters_per_grid);
  LOG_INFO<<"TerrainCDLOD: creating normal map done."<<endl;

  LOG_INFO<<"TerrainCDLOD: creating normal map texture ..."<<endl;
  auto normal_map_texture = createTexture(normal_map);
  LOG_INFO<<"TerrainCDLOD: creating normal map  texture done."<<endl;

  TextureParameters<int> params;
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  params.apply(normal_map_texture);

  TextureParameters<glm::vec4> params_vec4;
  params_vec4.set(GL_TEXTURE_BORDER_COLOR, glm::vec4(0,0,1,0));
  params_vec4.apply(normal_map_texture);

  return normal_map_texture;
}


TexturePtr TerrainCDLODBase::createHeightMapTexture(render_util::ElevationMap::ConstPtr hm_image)
{
  using namespace render_util;

  LOG_INFO<<"TerrainCDLOD: creating height map texture ..."<<endl;

  auto height_map_texture = createTextureExt(image::convert<half_float::half>(hm_image), true);

  CHECK_GL_ERROR();

  TextureParameters<int> params;
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  params.apply(height_map_texture);

  CHECK_GL_ERROR();

  return height_map_texture;
}


} // namespace render_util
