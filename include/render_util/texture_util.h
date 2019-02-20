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

#if 1
#ifndef RENDER_UTIL_TEXTURE_UTIL_H
#define RENDER_UTIL_TEXTURE_UTIL_H

#include <render_util/image.h>
#include <render_util/image_util.h>
#include <render_util/texture_manager.h>
#include <render_util/render_util.h>
#include <render_util/elevation_map.h>

#include <vector>
#include <type_traits>

namespace render_util
{
  //FIXME use ImageRGB
  ImageRGBA::Ptr createNormalMap(ImageGreyScale::ConstPtr bump_map, float bump_height_scale);

  TexturePtr createTexture(const unsigned char *data, int w, int h, int bytes_per_pixel, bool mipmaps);
  TexturePtr createFloatTexture1D(const float *data, size_t size, int num_components);
  TexturePtr createFloatTexture(const float *data, int w, int h, int num_components, bool mipmaps = false);
  TexturePtr createUnsignedIntTexture(const unsigned int *data, int w, int h);
//   unsigned int createTextureArray(const std::vector<render_util::ImageRGBA::ConstPtr> &textures, int mipmap_level);
  TexturePtr createTextureArray(const std::vector<const unsigned char*> &textures,
                                  int mipmap_levels, int texture_width, int bytes_per_pixel);

  void setTextureImage(TexturePtr texture,
                      const unsigned char *data,
                      int w,
                      int h,
                      int bytes_per_pixel,
                      bool mipmaps);


  template <typename T>
  void setTextureImage(TexturePtr texture, T image, bool mipmaps = true)
  {
    using ImageType = typename image::TypeFromPtr<T>::Type;
    static_assert(std::is_same<typename ImageType::ComponentType, unsigned char>::value);

    setTextureImage(texture,
                    image->getData(),
                    image->w(),
                    image->h(),
                    ImageType::BYTES_PER_PIXEL,
                    mipmaps);
  }

  template <typename T>
  TexturePtr createTextureArray(const std::vector<typename T::ConstPtr> &textures, int mipmap_levels = 0)
  {
    static_assert(std::is_same<typename T::ComponentType, unsigned char>::value);

    assert(!textures.empty());

    assert(textures[0]);
    const int texture_width = textures[0]->w();
    const unsigned texture_size = texture_width * texture_width * T::BYTES_PER_PIXEL;

    std::vector<const unsigned char*> texture_data(textures.size());
    for (unsigned i = 0; i < textures.size(); i++)
    {
      assert(textures[i]);
      assert(textures[i]->w() == texture_width);
      assert(textures[i]->h() == texture_width);
      assert(textures[i]->dataSize() == texture_size);
      texture_data[i] = textures[i]->data();
    }

    return createTextureArray(texture_data, mipmap_levels, texture_width, T::BYTES_PER_PIXEL);
  }

//   template <typename T>
//   unsigned int createTexture(typename Image<T>::ConstPtr image, bool mipmaps = true)
//   {
//     return createTexture(image->data(),
//                          image->w(),
//                          image->h(),
//                          render_util::Image<T>::BYTES_PER_PIXEL,
//                          mipmaps);
//   }


  inline TexturePtr createTexture(std::shared_ptr<const GenericImage> image, bool mipmaps = true)
  {
    return createTexture(image->data(),
                         image->w(),
                         image->h(),
                         image->numComponents(),
                         mipmaps);
  }


  template <typename T>
  TexturePtr createTexture(typename T::ConstPtr image, bool mipmaps = true)
  {
    static_assert(std::is_same<typename T::ComponentType, unsigned char>::value);
    return createTexture(image->data(),
                         image->w(),
                         image->h(),
                         T::BYTES_PER_PIXEL,
                         mipmaps);
  }

  template <typename T>
  TexturePtr createFloatTexture(T image, bool mipmaps)
  {
    static_assert(std::is_same<typename T::element_type::ComponentType, float>::value);
    return createFloatTexture(reinterpret_cast<const float*>(image->getData()),
                              image->w(),
                              image->h(),
                              T::element_type::NUM_COMPONENTS,
                              mipmaps);
  }


  TexturePtr createAmosphereThicknessTexture(TextureManager &texture_manager, std::string resource_path);
  TexturePtr createCurvatureTexture(TextureManager &texture_manager, std::string resource_path);

  ImageRGBA::Ptr createMapFarTexture(
      ImageGreyScale::ConstPtr type_map,
      const std::vector<ImageRGBA::ConstPtr> &textures,
//       ImageGreyScale::ConstPtr forest_map,
//       ImageRGBA::ConstPtr forest_texture,
      int type_map_meters_per_pixel,
      int meters_per_tile);

  ImageGreyScale::Ptr createTerrainLightMap(const ElevationMap&);

  Image<Normal>::Ptr createNormalMap(ElevationMap::ConstPtr elevation_map, float grid_scale);
}

#endif
#endif
