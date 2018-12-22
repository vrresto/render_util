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

#include "map_loader.h"
#include "tiff.h"
#include <FastNoise.h>
#include <render_util/elevation_map.h>
#include <render_util/image_loader.h>
#include <render_util/image_resample.h>
#include <render_util/image_util.h>
#include <render_util/map.h>
#include <render_util/texunits.h>
#include <render_util/texture_util.h>
#include <render_util/render_util.h>

#include <vector>
#include <iostream>

using namespace std;
using namespace render_util;

namespace
{


const glm::vec3 default_water_color = glm::vec3(0.140, 0.195, 0.230);
const float sea_level = 0;
const int type_map_meters_per_pixel = 200;
const int meters_per_tile = TerrainBase::TILE_SIZE_M;

enum
{
  TERRAIN_TYPE_WATER,
  TERRAIN_TYPE_GRASS,
  TERRAIN_TYPE_FIELD,
  TERRAIN_TYPE_FIELD2,
  TERRAIN_TYPE_FIELD3,
  TERRAIN_TYPE_FIELD4,
  TERRAIN_TYPE_FOREST,
  TERRAIN_TYPE_TUNDRA,
  TERRAIN_TYPE_TUNDRA2,
  TERRAIN_TYPE_ROCK,
  TERRAIN_TYPE_ICE
};


ImageRGBA::Ptr getTexture(const string &name)
{
  string path = render_util::getDataPath() + "/textures/" + name + ".tga";
  cout<<path<<endl;
  ImageRGBA::Ptr image(loadImageFromFile<ImageRGBA>(path));
  assert(image);
  return image;
}


bool readWaterAnimation(const string &filename, vector<char> &data)
{
  auto path = getDataPath() + "/water_textures/" + filename;
  return util::readFile(path, data);
}


void createWaterNormalMaps(render_util::WaterAnimation *water_animation,
                           render_util::MapTextures *map_textures)
{
  enum { MAX_ANIMATION_STEPS = 32 };

  printf("loading water textures...\n");


  vector<ImageRGBA::ConstPtr> normal_maps;
  vector<ImageGreyScale::ConstPtr> foam_masks;

  int i = 0;
  while (i < MAX_ANIMATION_STEPS)
  {

    char base_name[100];
//
    snprintf(base_name, sizeof(base_name), "WaterNoise%.2dDot3", i);
    auto filename = string(base_name) + ".tga";
    cout << "loading " << filename << endl;
    vector<char> data;
    if (!readWaterAnimation(filename, data))
    {
      break;
    }
    auto normal_map = render_util::loadImageFromMemory<ImageRGBA>(data);
    assert(normal_map);
//     dump(normal_map, base_name, loader->getDumpDir());
    normal_maps.push_back(normal_map);

    snprintf(base_name, sizeof(base_name), "WaterNoiseFoam%.2d", i);
    filename = string(base_name) + ".tga";
    cout << "loading " << filename << endl;;
    if (!readWaterAnimation(filename, data))
    {
      break;
    }
    auto foam_mask = render_util::loadImageFromMemory<ImageGreyScale>(data);
    assert(foam_mask);
//     dump(foam_mask, base_name, loader->getDumpDir());
    foam_masks.push_back(foam_mask);

    i++;
  }

  assert(normal_maps.size() == foam_masks.size());

  water_animation->createTextures(map_textures, normal_maps, foam_masks);
}


ImageGreyScale::Ptr generateTypeMap(ElevationMap::ConstPtr elevation_map)
{
  FastNoise noise_generator;
//   noise_generator.SetFrequency(0.4);

  Image<Normal>::Ptr normal_map = createNormalMap(elevation_map, 200);
  ImageGreyScale::Ptr type_map = image::create<unsigned char>(0, elevation_map->getSize());

  for (int y = 0; y < type_map->h(); y++)
  {
    for (int x = 0; x < type_map->w(); x++)
    {
      unsigned type = 0;
      float height = elevation_map->get(x,y);
      float noise = noise_generator.GetSimplex(x * 10, y * 10);

      if (height <= sea_level)
      {
        type = TERRAIN_TYPE_WATER;
      }
      else if (height < 500)
      {
        type = TERRAIN_TYPE_FOREST;
      }
      else if (height < 1800)
      {
        if (height < 700 && noise_generator.GetValueFractal(x * 16, y * 16) < 0.2)
        {
          type = TERRAIN_TYPE_GRASS;
        }
        else
        {
          if (noise > 0)
            type = TERRAIN_TYPE_TUNDRA;
          else
            type = TERRAIN_TYPE_TUNDRA2;
        }
      }
      else
        type = TERRAIN_TYPE_ICE;

      type_map->at(x,y) = type;
    }
  }
  
  for (int y = 0; y < type_map->h(); y++)
  {
    for (int x = 0; x < type_map->w(); x++)
    {
      float height = elevation_map->get(x,y);
      const Normal &normal = normal_map->at(x,y);
      unsigned type = type_map->at(x,y);

      if (type == TERRAIN_TYPE_FOREST && height < 200)
      {
        if (normal.z > 0.98)
        {
          float noise = noise_generator.GetValueFractal(x * 8, y * 8);
          if (noise > 0.0)
            type_map->at(x,y) = TERRAIN_TYPE_FIELD;
          else
            type_map->at(x,y) = TERRAIN_TYPE_FIELD3;

          if (noise_generator.GetValueFractal(x * 5, y * 5) > 0.0)
          {
            if (noise_generator.GetValueFractal(x * 8, y * 8) > 0)
              type_map->at(x,y) = TERRAIN_TYPE_FIELD2;
            else
              type_map->at(x,y) = TERRAIN_TYPE_FIELD4;
          }
        }
        if (normal.z < 0.9)
          type_map->at(x,y) = TERRAIN_TYPE_GRASS;
      }
      else if (type == TERRAIN_TYPE_FOREST)
      {
        if (noise_generator.GetValueFractal(x * 16, y * 16) +
            noise_generator.GetValueFractal(x * 2, y * 2) -
            noise_generator.GetValueFractal((x+10000) * 6, (y+10000) * 6) - 0.3 > 0.0)
        {
          type_map->at(x,y) = TERRAIN_TYPE_GRASS;
        }
        if (normal.z < 0.9)
          type_map->at(x,y) = TERRAIN_TYPE_GRASS;
      }


//       if (type == TERRAIN_TYPE_ICE && normal.z < 0.85)
//         type_map->at(x,y) = TERRAIN_TYPE_ROCK);

      if (normal.z < 0.7)
        type_map->at(x,y) = TERRAIN_TYPE_ROCK;

    }

  }


  return type_map;
}

ImageRGBA::Ptr loadTexture(unsigned type, float &scale)
{
  string name;
  
  switch (type)
  {
    case TERRAIN_TYPE_GRASS:
      name = "grass";
      scale = 0.5;
      break;
    case TERRAIN_TYPE_FIELD:
      name = "field";
      scale = 0.5;
      break;
    case TERRAIN_TYPE_FIELD2:
      name = "field2";
      scale = 0.5;
      break;
    case TERRAIN_TYPE_FIELD3:
      name = "field3";
      scale = 0.5;
      break;
    case TERRAIN_TYPE_FIELD4:
      name = "field4";
      scale = 0.5;
      break;
    case TERRAIN_TYPE_FOREST:
      name = "forest";
      break;
    case TERRAIN_TYPE_WATER:
      name = "water";
      break;
    case TERRAIN_TYPE_TUNDRA:
      name = "tundra";
      scale = 0.25;
      break;
    case TERRAIN_TYPE_TUNDRA2:
      name = "tundra2";
      break;
    case TERRAIN_TYPE_ICE:
      name = "ice";
      break;
    case TERRAIN_TYPE_ROCK:
      name = "rock";
      break;
    default:
      assert(0);
      break;
  }
  
  string path = getDataPath() + "/textures/" + name + ".tga";

  ImageRGBA::Ptr image(loadImageFromFile<ImageRGBA>(path));
  
  if (!image)
    cout<<"failed to load "<<path<<endl;

  assert(image);
  
  return image;
};

void loadTextures(ImageGreyScale::Ptr type_map, render_util::MapTextures &map_textures)
{
  vector<ImageRGBA::ConstPtr> textures;
  vector<float> scales;
  map<unsigned, unsigned> mapping;

  type_map->forEach([&] (unsigned char &value)
  {
    if (mapping.find(value) == mapping.end())
    {
      float scale = 1;
      ImageRGBA::ConstPtr texture = loadTexture(value, scale);
      textures.push_back(texture);
      scales.push_back(scale);
      mapping.insert(make_pair(value, textures.size()-1));
    }

    value = mapping[value];
  });

  map_textures.setTextures(textures, scales);

  ImageRGBA::Ptr far_texture =
    createMapFarTexture(type_map,
                        textures,
                        type_map_meters_per_pixel,
                        meters_per_tile);

  map_textures.setTexture(TEXUNIT_TERRAIN_FAR, far_texture);
}

vector<ImageRGBA::ConstPtr> getBeachTextures()
{
  vector<ImageRGBA::ConstPtr> beach;

  ImageRGBA::ConstPtr beach_foam = getTexture("BeachFoam");
  assert(beach_foam);
  beach.push_back(beach_foam);

  ImageRGBA::ConstPtr beach_surf = getTexture("BeachSurf");
  assert(beach_surf);
  beach.push_back(beach_surf);

  ImageRGBA::ConstPtr beach_land = getTexture("BeachLand");
  assert(beach_land);
  beach.push_back(beach_land);

  return beach;
}

ImageGreyScale::ConstPtr createWaterMapSimple(ImageGreyScale::Ptr type_map)
{
  ImageGreyScale::Ptr map = image::create<unsigned char>(0, type_map->size());

  for (int y = 0; y < type_map->h(); y++)
  {
    for (int x = 0; x < type_map->w(); x++)
    {
      if (type_map->at(x,y) == TERRAIN_TYPE_WATER)
        map->at(x,y) = 255;
    }
  }

  map = downSample(map, 2);
//   map = upSample(map, 4);
  
  return map;
}

ImageRGBA::Ptr getForestFarTexture()
{
  return getTexture("forest_far");
}

vector<ImageRGBA::ConstPtr> getForestLayers()
{
  vector<ImageRGBA::ConstPtr> textures;
  
  for (int i = 0; i <= 4; i++)
  {
    cout<<"getForestTexture() - layer "<<i<<endl;

    auto name = string("forest") + to_string(i);

    ImageRGBA::Ptr texture = getTexture(name);
    if (!texture)
      continue;

    textures.push_back(texture);
  }

  return textures;
}

ImageGreyScale::Ptr createForestMap(ImageGreyScale::ConstPtr type_map)
{
  auto map = image::clone(type_map);

  map->forEach([] (unsigned char &pixel)
  { 
    if (pixel == TERRAIN_TYPE_FOREST)
      pixel = 255;
    else
      pixel = 0;
  });

  return map;
}


} // namespace


void MapLoader::loadMap(render_util::Map &map,
                        bool load_terrain,
                        render_util::ElevationMap::Ptr *elevation_map_out,
                        render_util::ElevationMap::Ptr *elevation_map_base_out)
{

  string height_map_path = getDataPath() + "/nz.tiff";

  Image<float>::Ptr height_map = loadTiff(height_map_path.c_str());

  height_map = downSample(height_map, 2);

  {
    const float max_height = 3000;
    
    ImageGreyScale::Ptr tmp(new ImageGreyScale(height_map->size()));

    for (int y = 0; y < tmp->h(); y++)
    {
      for (int x = 0; x < tmp->w(); x++)
      {
        float rel_height = clamp(height_map->at(x,y) / max_height, 0.f, 1.f);
        tmp->at(x,y) = rel_height * 255;
      }
    }
//     saveImageToFile("../height_map_dump.tga", tmp.get());
//     exit(0);
  }
  
  height_map->forEach([](float &height)
  {
    if (height < sea_level)
      height = sea_level;
  });

  assert(height_map);

  const uint elevation_map_width = height_map->w();
  const uint elevation_map_height = height_map->h();

  float size_mb = ((float)height_map->dataSize()) / 1024 /  1024;
  cout<<"elevation data size: "<<size_mb<<" MB"<<endl;

//   float highest_elevation = 0;
//   float lowest_elevation = 0;
//   for (unsigned i = 0; i < ?; i++)
//   {
// //     cout<<"elevation: "<<height_map->getData()[i]<<endl;
// 
//     if (height_map->getData()[i] > highest_elevation)
//       highest_elevation = height_map->getData()[i];
// 
//     if (height_map->getData()[i] < lowest_elevation)
//       lowest_elevation = height_map->getData()[i];
//   }
//   cout<<"lowest elevation: "<<lowest_elevation<<endl;
//   cout<<"highest elevation: "<<highest_elevation<<endl;
  
  ImageGreyScale::Ptr type_map;


  {
    if (load_terrain)
    {
      assert(map.terrain);
      map.terrain->build(height_map);
    }

    type_map = generateTypeMap(height_map);

    if (elevation_map_out)
      *elevation_map_out = height_map;
  }

//   saveImageToFile("../type_map.tga", type_map.get());

  auto forest_map = createForestMap(type_map);
  assert(forest_map);
  map.textures->setForestMap(forest_map);

  ImageGreyScale::ConstPtr water_map = createWaterMapSimple(type_map);

  loadTextures(type_map, *map.textures);

  createWaterNormalMaps(map.water_animation.get(), map.textures.get());

  map.textures->setTypeMap(type_map);
  map.textures->setTexture(TEXUNIT_WATER_MAP_SIMPLE, water_map);
  map.textures->setWaterColor(default_water_color * glm::vec3(0.7));
  map.textures->setForestLayers(getForestLayers());
  map.textures->setTexture(TEXUNIT_FOREST_FAR, getForestFarTexture());
  map.textures->setBeach(getBeachTextures());
  map.textures->setTexture(TEXUNIT_SHALLOW_WATER, getTexture("shallow_water"));

//   saveImageToFile("forest_far.tga", getForestFarTexture().get());

  map.size = type_map->size() * glm::ivec2(200);
}
