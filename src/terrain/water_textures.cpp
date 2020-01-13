#include "water_textures.h"
#include <render_util/terrain_base.h>
#include <render_util/texunits.h>
#include <render_util/gl_binding/gl_functions.h>

namespace
{


const float water_map_chunk_size = 128;
const float water_map_crop_size = 4.0;
const float water_map_chunk_scale = 1.0 / (water_map_chunk_size /
                                           (water_map_chunk_size - (water_map_crop_size)));
const float water_map_shift_unit = 1.0 / (water_map_chunk_size - (water_map_crop_size));
const float water_map_chunk_shift_m = 1 * (water_map_shift_unit / 2);

const float water_map_chunk_size_m = 1600 * 4;

const glm::vec2 water_map_table_shift_m = glm::vec2(0, 200);


}


namespace render_util::terrain
{

WaterTextures::WaterTextures(const TextureManager &texture_manager,
                              render_util::TerrainBase::BuildParameters &params) :
    m_texture_manager(texture_manager)
{
}


void WaterTextures::loadLayer(Layer &layer,
                              const TerrainBase::Loader::Layer &loader,
                              bool is_base_layer) const
{
  std::vector<ImageGreyScale::Ptr> chunks;
  Image<unsigned int>::Ptr table;

  try
  {
    loader.loadWaterMap(chunks, table);
  }
  catch (...)
  {
    return;
  }

  auto chunks_texture = createTextureArray(chunks);
  TextureParameters<int> chunks_params;
  chunks_params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  chunks_params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  chunks_params.apply(chunks_texture);

  auto table_float = image::convert<float>(table);
  auto table_texture = createFloatTexture(
                                reinterpret_cast<const float*>(table_float->data()),
                                table_float->w(),
                                table_float->h(),
                                1);
  TextureParameters<int> table_params;
  table_params.set(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  table_params.set(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  table_params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  table_params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  table_params.apply(table_texture);

  WaterMap map
  {
    .texunit_table = is_base_layer ? TEXUNIT_WATER_MAP_TABLE_BASE : TEXUNIT_WATER_MAP_TABLE,
    .texunit = is_base_layer ? TEXUNIT_WATER_MAP_BASE : TEXUNIT_WATER_MAP,
    .texture_table = table_texture,
    .texture = chunks_texture,
    .table_size_m = glm::vec2(table->size()) * water_map_chunk_size_m,
    .table_shift_m = water_map_table_shift_m,
    .chunk_size_m = water_map_chunk_size_m,
    .chunk_scale = glm::vec2(water_map_chunk_scale),
    .chunk_shift_m = glm::vec2(water_map_chunk_shift_m) + water_map_table_shift_m,
  };

  layer.water_map = map;
  layer.has_water_map = true;
}


}
