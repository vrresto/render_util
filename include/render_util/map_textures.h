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

#ifndef RENDER_UTIL_MAP_TEXTURES_H
#define RENDER_UTIL_MAP_TEXTURES_H

#include <vector>
#include <render_util/texture_manager.h>
#include <render_util/texture_util.h>
#include <render_util/image.h>
#include <render_util/shader.h>

#include <glm/glm.hpp>

namespace render_util
{
  class MapTextures
  {
    const TextureManager &m_texture_manager;

    struct Private;
    Private *p =  nullptr;

  public:
    MapTextures(const TextureManager &texture_manager);
    ~MapTextures();

    void setWaterColor(const glm::vec3 &color);
    void setTypeMap(ImageGreyScale::ConstPtr type_map);
    void setWaterTypeMap(ImageGreyScale::ConstPtr type_map);
    void setBeach(std::vector<ImageRGBA::ConstPtr> textures);

    void setForestMap(ImageGreyScale::ConstPtr texture);
    void setForestLayers(const std::vector<ImageRGBA::ConstPtr> &textures);

    void setTextures(const std::vector<ImageRGBA::ConstPtr> &textures,
                     const std::vector<float> &texture_scale);

    void setWaterMap(const std::vector<ImageGreyScale::ConstPtr> &chunks,
                     Image<unsigned int>::ConstPtr table);

    void setTexture(unsigned texunit, TexturePtr texture);

    void bind(TextureManager&);
    void setUniforms(ShaderProgramPtr program);

    template <class T>
    void setTexture(unsigned texunit, std::shared_ptr<const T> image)
    {
      setTexture(texunit, createTexture<T>(image));
    }

    template <class T>
    void setTexture(unsigned texunit, std::shared_ptr<T> image)
    {
      setTexture(texunit, createTexture<T>(image));
    }

    template <class T>
    void setTextureArray(unsigned texunit, const std::vector<std::shared_ptr<const T>> &images)
    {
      setTexture(texunit, createTextureArray<T>(images));
    }



    TexturePtr getBaseMapTexture();

  };

}

#endif
