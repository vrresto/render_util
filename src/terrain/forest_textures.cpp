#include "forest_textures.h"
#include <render_util/image_resample.h>
#include <render_util/gl_binding/gl_functions.h>


namespace render_util::terrain
{


ForestTextures::ForestTextures(const TextureManager &texture_manager,
                               TerrainBase::BuildParameters &params) :
  m_texture_manager(texture_manager)
{
  auto &loader = params.loader;

  {
    auto forest_layers = loader.loadForestLayers();

    auto resampled = resampleImages(forest_layers, getMaxWidth(forest_layers));
    m_forest_layers_texture = createTextureArray(resampled);

    TextureParameters<int> params;
    params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //   params.set(GL_TEXTURE_LOD_BIAS, 2.0);
    params.apply(m_forest_layers_texture);
  }

  {
    auto forest_far = loader.loadForestFarTexture();
    m_forest_far_texture = createTexture(forest_far);

    TextureParameters<int> params;
    params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    params.apply(m_forest_far_texture);
  }
}


void ForestTextures::bind(TextureManager &tm)
{
  tm.bind(TEXUNIT_FOREST_LAYERS, m_forest_layers_texture);
  tm.bind(TEXUNIT_FOREST_FAR, m_forest_far_texture);
}


void ForestTextures::unbind(TextureManager &tm)
{
  tm.unbind(TEXUNIT_FOREST_LAYERS, m_forest_layers_texture->getTarget());
  tm.unbind(TEXUNIT_FOREST_FAR, m_forest_far_texture->getTarget());
}


void ForestTextures::setUniforms(ShaderProgramPtr program) const
{
  program->setUniformi("sampler_forest_layers",
                       m_texture_manager.getTexUnitNum(TEXUNIT_FOREST_LAYERS));
  program->setUniformi("sampler_forest_far",
                       m_texture_manager.getTexUnitNum(TEXUNIT_FOREST_FAR));
}

void ForestTextures::loadLayer(TerrainLayer &layer,
                              const TerrainBase::Loader::Layer &loader,
                              bool is_base_layer) const
{
  {
    auto forest_map = loader.loadForestMap();
    auto texture = createTexture(forest_map, true);

    TextureParameters<int> params;

    params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  //   params.set(GL_TEXTURE_LOD_BIAS, 10.0);

    params.apply(texture);

    TerrainTextureMap map
    {
      .texunit = is_base_layer ? TEXUNIT_FOREST_MAP_BASE : TEXUNIT_FOREST_MAP,
      .resolution_m = TerrainBase::GRID_RESOLUTION_M,
      .size_m = forest_map->getSize() * TerrainBase::GRID_RESOLUTION_M,
      .size_px = forest_map->getSize(),
      .texture = texture,
      .name = "forest_map",
    };

    layer.texture_maps.push_back(map);
  }

}


}
