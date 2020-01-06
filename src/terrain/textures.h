#ifndef RENDER_UTIL_TERRAIN_TEXTURES_H
#define RENDER_UTIL_TERRAIN_TEXTURES_H

#include "terrain_layer.h"

#include <vector>

namespace render_util::terrain
{


class Textures
{
  std::vector<terrain::TerrainTextureMap> m_texture_maps;
  std::vector<terrain::TerrainTextureMap> m_base_texture_maps;

protected:
  void addTextureMap(const terrain::TerrainTextureMap &map)
  {
    m_texture_maps.push_back(map);
  }

  void addBaseTextureMap(const TerrainTextureMap &map)
  {
    m_base_texture_maps.push_back(map);
  }

public:
  const std::vector<terrain::TerrainTextureMap> &getTextureMaps() const { return m_texture_maps; }
  const std::vector<terrain::TerrainTextureMap> &getBaseTextureMaps() const { return m_base_texture_maps; }

  virtual const ShaderParameters &getShaderParameters() const = 0;
  virtual void bind(TextureManager&) = 0;
  virtual void unbind(TextureManager&) = 0;
  virtual void setUniforms(ShaderProgramPtr program) const {}
};


}

#endif
