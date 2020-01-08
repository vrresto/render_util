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

#include "land_textures.h"
#include <log.h>
#include <render_util/image_resample.h>
#include <render_util/texture_util.h>
#include <render_util/gl_binding/gl_functions.h>

#include <set>
#include <deque>

using namespace render_util;
using namespace render_util::terrain;
using std::endl;

namespace
{


Map createTypeMap(TerrainBase::TypeMap::ConstPtr type_map_in,
                                const std::map<unsigned, glm::uvec3> &mapping,
                                std::string name,
                                unsigned int texunit)
{
  auto type_map = std::make_shared<ImageRGBA>(type_map_in->getSize());

  for (int y = 0; y < type_map->h(); y++)
  {
    for (int x = 0; x < type_map->w(); x++)
    {
      unsigned int orig_index = type_map_in->get(x,y) & 0x1F;

      glm::uvec3 new_index(0);

      auto it = mapping.find(orig_index);
      if (it != mapping.end())
      {
        new_index = it->second;
      }
      else
      {
        for (int i = 0; i < 4; i++)
        {
          auto it = mapping.find(orig_index - (orig_index % 4) + i);
          if (it != mapping.end())
          {
            new_index = it->second;
            break;
          }
          else
          {
            new_index.x = 255;
          }
        }
      }

      assert(new_index.y <= 0x1F+1);
      type_map->at(x,y,0) = new_index.x;
      type_map->at(x,y,1) = new_index.y;
      type_map->at(x,y,2) = new_index.z;
      type_map->at(x,y,3) = 255;
    }
  }

  TexturePtr texture;
  {
    texture = render_util::createTexture(type_map, false);
    TextureParameters<int> params;
    params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    params.set(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    params.set(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    params.apply(texture);
  }

  Map map
  {
    .texunit = texunit,
    .resolution_m = LandTextures::TYPE_MAP_RESOLUTION_M,
    .size_m = type_map->getSize() * LandTextures::TYPE_MAP_RESOLUTION_M,
    .size_px = type_map->getSize(),
    .texture = texture,
    .name = "type_map",
  };

  return map;
}


void createTextureArrays(
    const std::vector<std::shared_ptr<render_util::ImageResource>> &textures_in,
    const std::vector<float> &texture_scale_in,
    double max_texture_scale,
    std::array<TexturePtr, render_util::MAX_TERRAIN_TEXUNITS> &arrays_out,
    std::map<unsigned, glm::uvec3> &mapping_out)
{
  using namespace glm;
  using namespace std;
  using namespace render_util;
  using TextureArray = std::vector<ScaledImageResource>;

  std::array<TextureArray, MAX_TERRAIN_TEXUNITS> texture_arrays;
  map<unsigned, glm::uvec3> mapping;

  std::set<size_t> all_texture_sizes;
  for (auto texture : textures_in)
  {
    if (!texture)
      continue;
    assert(texture->w() == texture->h());
    all_texture_sizes.insert(texture->w());
  }

  std::deque<size_t> texture_sizes;
  for (auto size : all_texture_sizes)
    texture_sizes.push_back(size);

  while (texture_sizes.size() > MAX_TERRAIN_TEXUNITS)
    texture_sizes.pop_front();

  auto smallest_size = texture_sizes.front();

  std::unordered_map<int, int> array_index_for_size;
  for (size_t i = 0; i < texture_sizes.size(); i++)
    array_index_for_size[texture_sizes.at(i)] = i;

  for (int i = 0; i < textures_in.size(); i++)
  {
    float scale = texture_scale_in.at(i);
    assert(scale != 0);
    assert(fract(scale) == 0);
    assert(scale <= max_texture_scale);
    scale += 128;
    assert(scale >= 0);
    assert(scale <= 255);

    auto image = textures_in.at(i);
    if (!image)
      continue;


    ScaledImageResource scaled_image;
    scaled_image.resource = image;

    while (scaled_image.getScaledSize().x < smallest_size)
    {
//       auto biggest_size = texture_sizes.back();
//       LOG_INFO<<"image->w(): "<<image->w()<<", smallest_size: "
//         <<smallest_size<<", biggest_size: "<<biggest_size<<endl;
      scaled_image.scale_exponent++;
    }

    auto index = array_index_for_size.at(scaled_image.getScaledSize().x);

    texture_arrays.at(index).push_back(scaled_image);
    mapping.insert(make_pair(i, glm::uvec3{index, texture_arrays[index].size()-1, scale}));
  }

  mapping_out = mapping;

  for (int i = 0; i < texture_arrays.size(); i++)
  {
    CHECK_GL_ERROR();

    auto &textures = texture_arrays.at(i);
    if (textures.empty())
      continue;

    LOG_TRACE<<"array: "<<i<<endl;
    arrays_out.at(i) = render_util::createTextureArray(textures);

    CHECK_GL_ERROR();
  }
}


} // namespace


namespace render_util::terrain
{


LandTextures::LandTextures(const TextureManager &texture_manager,
                           TerrainBase::BuildParameters &params) :
    m_texture_manager(texture_manager)
{
  using namespace glm;
  using namespace std;
  using namespace render_util;
  using TextureArray = vector<ImageRGBA::ConstPtr>;

  auto &loader = params.loader;

  assert(loader.getLandTextures().size() == loader.getLandTexturesScale().size());

  LOG_INFO << "Creating land textures ..." << endl;

  createTextureArrays(loader.getLandTextures(),
                      loader.getLandTexturesScale(),
                      MAX_TEXTURE_SCALE,
                      m_textures,
                      m_mapping);

#if 0
  if (!textures_nm.empty())
  {
    m_enable_normal_maps = true;
    m_shader_params.set("enable_terrain_detail_nm", true);
    assert(textures.size() == textures_nm.size());

    createTextureArrays<ImageRGB>(textures_nm,
                                  texture_scale,
                                  type_map_,
                                  MAX_TEXTURE_SCALE,
                                  m_textures_nm,
                                  m_type_map_texture_nm,
                                  base_type_map_,
                                  m_base_type_map_texture_nm);
  }
#endif

  for (int i = 0; i < m_textures.size(); i++)
  {
    CHECK_GL_ERROR();

    if (!m_textures.at(i))
      continue;

    m_shader_params.set(string( "enable_terrain") + to_string(i), true);
  }

  for (int i = 0; i < m_textures_nm.size(); i++)
  {
    CHECK_GL_ERROR();

    if (!m_textures_nm.at(i))
      continue;

    m_shader_params.set(string( "enable_terrain_detail_nm") + to_string(i), true);
  }

  LOG_INFO << "Creating land textures ... done." << endl;
}


void LandTextures::loadLayer(Layer &layer, const TerrainBase::Loader::Layer &loader,
                             bool is_base_layer) const
{
  auto texunit = is_base_layer ? TEXUNIT_TYPE_MAP_BASE : TEXUNIT_TYPE_MAP;
  auto type_map = createTypeMap(loader.loadTypeMap(), m_mapping, "type_map", texunit);
  layer.maps.push_back(type_map);
}


void LandTextures::bind(TextureManager &tm)
{
  using namespace render_util;

  for (int i = 0; i < m_textures.size(); i++)
  {
    CHECK_GL_ERROR();

    auto textures = m_textures.at(i);
    if (!textures)
      continue;

    auto texunit = TEXUNIT_TERRAIN + i;
    assert(texunit < TEXUNIT_NUM);

    tm.bind(texunit, textures);

    CHECK_GL_ERROR();
  }

  if (m_enable_normal_maps)
  {
//     assert(m_type_map_texture_nm);
//     tm.bind(TEXUNIT_TYPE_MAP_NORMALS, m_type_map_texture_nm);
// 
//     for (int i = 0; i < m_textures_nm.size(); i++)
//     {
//       CHECK_GL_ERROR();
// 
//       auto textures = m_textures_nm.at(i);
//       if (!textures)
//         continue;
// 
//       auto texunit = TEXUNIT_TERRAIN_DETAIL_NM0 + i;
//       assert(texunit < TEXUNIT_NUM);
// 
//       tm.bind(texunit, textures);
// 
//       CHECK_GL_ERROR();
//     }
  }
}


void LandTextures::unbind(TextureManager &tm)
{
  LOG_TRACE<<endl;

  using namespace render_util;

  for (int i = 0; i < m_textures.size(); i++)
  {
    CHECK_GL_ERROR();

    auto texture = m_textures.at(i);
    if (!texture)
      continue;

    auto texunit = TEXUNIT_TERRAIN + i;
    assert(texunit < TEXUNIT_NUM);

    tm.unbind(texunit, texture->getTarget());

    CHECK_GL_ERROR();
  }

  if (m_enable_normal_maps)
  {
//     tm.unbind(TEXUNIT_TYPE_MAP_NORMALS, m_type_map_texture_nm->getTarget());
// 
//     for (int i = 0; i < m_textures_nm.size(); i++)
//     {
//       CHECK_GL_ERROR();
// 
//       auto texture = m_textures_nm.at(i);
//       if (!texture)
//         continue;
// 
//       auto texunit = TEXUNIT_TERRAIN_DETAIL_NM0 + i;
//       assert(texunit < TEXUNIT_NUM);
// 
//       tm.unbind(texunit, texture->getTarget());
// 
//       CHECK_GL_ERROR();
//     }
  }
}


} // namespace render_util::terrain
