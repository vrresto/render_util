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


namespace
{


unsigned int gatherMaterials(render_util::TerrainBase::MaterialMap::ConstPtr map,
                             glm::ivec2 begin,
                             glm::ivec2 size)
{
  using namespace glm;

  auto end = begin + size;

  begin = clamp(begin, ivec2(0), map->getSize());
  end = clamp(end, ivec2(0), map->getSize());

  unsigned int material = 0;

  for (int y = begin.y; y < end.y; y++)
  {
    for (int x = begin.x; x < end.x; x++)
    {
      material |= map->get(x,y);
    }
  }

  return material;
}




} // namespace


namespace render_util
{


struct TerrainCDLODBase::MaterialMap::Map
{
  render_util::TerrainBase::MaterialMap::ConstPtr map;
  glm::vec2 origin_m = glm::vec2(0);
  float resolution_m = 0;

  unsigned int getMaterialID(const Rect &area)
  {
    assert(resolution_m);

    auto area_origin = (area.origin - origin_m);
    assert(fract(area_origin / resolution_m) == glm::vec2(0));

    auto area_origin_px = glm::ivec2(area_origin / resolution_m);

    assert(fract(area.extent / resolution_m) == glm::vec2(0));
    auto area_extent_px = area.extent / resolution_m;

    return gatherMaterials(map, area_origin_px, area_extent_px);
  }
};


TerrainCDLODBase::MaterialMap::MaterialMap(const TerrainBase::BuildParameters &params)
{
  assert(params.textures.detail_layer);
  m_map = createMap(*params.textures.detail_layer);

  if (params.textures.base_layer)
    m_base_map = createMap(*params.textures.base_layer);
}


TerrainCDLODBase::MaterialMap::~MaterialMap()
{
}


std::unique_ptr<TerrainCDLODBase::MaterialMap::Map>
TerrainCDLODBase::MaterialMap::createMap(const TerrainBase::Textures::Layer &layer)
{
  auto map = std::make_unique<Map>();
  map->map = layer.material_map;
  map->resolution_m = layer.resolution_m;
  map->origin_m = layer.origin_m;
  return map;
}


unsigned int TerrainCDLODBase::MaterialMap::getMaterialID(const Rect &area) const
{
  auto material = m_map->getMaterialID(area);
  if (m_base_map)
    material |= m_base_map->getMaterialID(area);

  if (!material)
    material = MaterialID::WATER;

  return material;
}


TexturePtr TerrainCDLODBase::createNormalMapTexture(render_util::ElevationMap::ConstPtr map,
                                                    int meters_per_grid)
{
  using namespace render_util;

  LOG_INFO<<"TerrainCDLOD: creating normal map ..."<<endl;
  auto normal_map = createNormalMap(map, meters_per_grid);
  LOG_INFO<<"TerrainCDLOD: creating normal map done."<<endl;

  auto normal_map_texture =
    createFloatTexture(reinterpret_cast<const float*>(normal_map->getData()),
                      map->getWidth(),
                      map->getHeight(),
                      3,
                      true);
  LOG_INFO<<"TerrainCDLOD: creating normal map  texture done."<<endl;

  TextureParameters<int> params;
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
