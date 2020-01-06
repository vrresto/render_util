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

#ifndef RENDER_UTIL_TERRAIN_BASE_H
#define RENDER_UTIL_TERRAIN_BASE_H

#include <render_util/shader.h>
#include <render_util/camera.h>
#include <render_util/elevation_map.h>
#include <render_util/texture_manager.h>
#include <render_util/texture_util.h>
#include <factory.h>

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace render_util
{
  class TerrainBase
  {
  public:
    static constexpr int TILE_SIZE_M = 1600;
    static constexpr int GRID_RESOLUTION_M = 200;

    struct MaterialID
    {
      static constexpr unsigned int LAND = 1;
      static constexpr unsigned int WATER = 1 << 1;
      static constexpr unsigned int FOREST = 1 << 2;
      static constexpr unsigned int ALL = LAND | WATER | FOREST;
    };

    using MaterialMap = Image<unsigned int>;
    using TypeMap = ImageGreyScale;

    struct Client
    {
      virtual void setActiveProgram(ShaderProgramPtr) = 0;
    };

//     struct Textures
//     {
//       struct Layer
//       {
//         glm::vec2 origin_m = glm::vec2(0);
//         unsigned int resolution_m = 0;
//         ElevationMap::ConstPtr height_map;
//         TerrainBase::TypeMap::ConstPtr type_map;
//         ImageGreyScale::ConstPtr forest_map;
//         MaterialMap::ConstPtr material_map;
//       };
// 
//       std::unique_ptr<Layer> base_layer;
//       std::unique_ptr<Layer> detail_layer;
// 
//       std::vector<ImageRGBA::Ptr> textures;
//       std::vector<ImageRGB::Ptr> textures_nm;
//       std::vector<float> texture_scale;
//       ImageRGBA::ConstPtr far_texture;
//       std::vector<ImageRGBA::ConstPtr> forest_layers;
//     //   ImageRGBA::ConstPtr forest_far;
//     };


    struct Loader
    {
      struct Layer
      {
        virtual glm::vec2 getOriginM() const = 0;
        virtual unsigned int getResolutionM() const = 0;
        virtual ElevationMap::Ptr loadHeightMap() const = 0;
        virtual TerrainBase::TypeMap::Ptr loadTypeMap() const = 0;
        virtual ImageGreyScale::Ptr loadForestMap() const = 0;
        virtual MaterialMap::Ptr loadMaterialMap() const = 0;
      };

      virtual bool hasBaseLayer() const = 0;

      virtual const Layer &getDetailLayer() const = 0;
      virtual const Layer &getBaseLayer() const = 0;

      virtual const std::vector<std::shared_ptr<ImageResource>> &getLandTextures() const = 0;
      virtual const std::vector<std::shared_ptr<ImageResource>> &getLandTexturesNM() const = 0;
      virtual const std::vector<float> &getLandTexturesScale() const = 0;

      virtual std::vector<ImageRGBA::Ptr> loadForestLayers() const = 0;
      virtual ImageRGBA::Ptr loadForestFarTexture() const = 0;

//       ImageRGBA::ConstPtr far_texture;
    //   ImageRGBA::ConstPtr forest_far;
    };


    struct BuildParameters
    {
      const ShaderParameters &shader_parameters;
      const Loader &loader;
    };

    virtual ~TerrainBase() {}

    virtual void build(BuildParameters&) = 0;

    virtual void draw(Client *client = nullptr) = 0;
    virtual void update(const Camera &camera, bool low_detail) {}
    virtual void setDrawDistance(float dist) {}
    virtual std::vector<glm::vec3> getNormals() { return {}; }
    virtual TexturePtr getNormalMapTexture() { return nullptr; }
    virtual void setProgramName(std::string) {}
    virtual void setBaseMapOrigin(glm::vec2 origin) {}
  };

  using TerrainFactory = util::Factory<TerrainBase, TextureManager&, const ShaderSearchPath&>;

  template <class T>
  TerrainFactory makeTerrainFactory()
  {
    return [] (TextureManager &tm, const ShaderSearchPath &shader_search_path)
    {
      return std::make_shared<T>(tm, shader_search_path);
    };
  }
}

#endif
