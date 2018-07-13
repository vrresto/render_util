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

#include <FastNoise.h>
#include <render_util/map_textures.h>
#include <render_util/texture_util.h>
#include <render_util/texunits.h>
#include <render_util/image_loader.h>
#include <render_util/image_resample.h>
#include <render_util/image_util.h>
#include <render_util/image.h>
#include <util.h>

#include <cassert>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <GL/gl.h>

#include <gl_wrapper/gl_functions.h>

#ifdef max
#undef max
#endif

using namespace gl_wrapper::gl_functions;
using namespace std;

namespace
{

using namespace engine;

const float water_map_chunk_size = 128;
const float water_map_crop_size = 4.0;
const float water_map_scale = water_map_chunk_size / (water_map_chunk_size - (water_map_crop_size));
const float water_map_shift_unit = 1.0 / (water_map_chunk_size - (water_map_crop_size));
const float water_map_shift = 1 * (water_map_shift_unit / 2);

// const int texture_size = 512;
const int texture_size = 1024;
// const int texture_size = 2048;


struct TextureBinding
{
  string uniform_name;
  unsigned int texture_unit = 0;
  TexturePtr texture;

  TextureBinding(const string &name, unsigned int texture_unit) :
    uniform_name(name), texture_unit(texture_unit) {}
};


class Material
{
  vector<TextureBinding> m_bindings;
  unordered_map<unsigned int, TextureBinding*> m_map;
  TextureManager &m_texture_manager;

public:
  Material(TextureManager &texture_manager) : m_texture_manager(texture_manager) {}

  void setTexture(unsigned int texture_unit, TexturePtr texture)
  {
    TextureBinding *b = m_map[texture_unit];
    if (!b)
    {
      string uniform_name = string("sampler_") + getTexUnitName(texture_unit);
      m_bindings.push_back(TextureBinding(uniform_name, texture_unit));
      b = &m_bindings.back();
      m_map[texture_unit] = b;
    }
    assert(b);
    b->texture = texture;
  }

  void setUniforms(ShaderProgramPtr program)
  {
    for (TextureBinding &b : m_bindings)
    {
      program->setUniformi(b.uniform_name, m_texture_manager.getTexUnitNum(b.texture_unit));
    }
  }

  void bind()
  {
    for (TextureBinding &b : m_bindings)
    {
      m_texture_manager.bind(b.texture_unit, b.texture);
    }
  }
};


TexturePtr createNoiseTexture()
{
  auto image = image::create<unsigned char>(0, glm::ivec2(2048));

  const double pi = util::PI;

  const double x1 = 0;
  const double y1 = 0;
  const double x2 = 800;
  const double y2 = 800;

//   siv::PerlinNoise noise_generator;
  FastNoise noise_generator;
  noise_generator.SetFrequency(0.4);

  for (int y = 0; y < image->h(); y++)
  {
    for (int x = 0; x < image->w(); x++)
    {
      double s = (double)x / image->w();
      double t = (double)y / image->h();
      double dx=x2-x1;
      double dy=y2-y1;

      double nx=x1+cos(s*2*pi)*dx/(2*pi);
      double ny=y1+cos(t*2*pi)*dy/(2*pi);
      double nz=x1+sin(s*2*pi)*dx/(2*pi);
      double nw=y1+sin(t*2*pi)*dy/(2*pi);
      
//       glm::dvec4 params(nx, ny, nz, nw);
//       glm::vec2 params(s, t);
      
//       double value = glm::perlin(params);
      
//       double value = noise_generator.octaveNoise(nx, ny, nz, nw);
      
      double value = noise_generator.GetSimplex(nx, ny, nz, nw);
     
//       double value = glm::simplex(params);
//       value = glm::clamp(value, 0.0, 1.0);
      
//       cout<<"value:"<<value<<endl

//       assert(value <= 1);
      
      

//       assert(value < 0.006);

      assert(!isnan(value));
      assert(value >= -1);
      assert(value <= 1);

      value += 1;
      value /= 2;

//       value = glm::clamp(value, 0.0f, 1.0f);
      
//       value *= 0.0;
      
      value *= 255;

      image->at(x,y) = value;
    }
  }
  
//   image->setPixel(0, 0, 255);
//   image->setPixel(0, 1, 255);
//   image->setPixel(1, 0, 255);
//   image->setPixel(1, 1, 255);

//   saveImageToFile("noise.tga", image.get());
  
  TexturePtr texture = createTexture<ImageGreyScale>(image);

  TextureParameters<int> params;
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  params.apply(texture);

  return texture;
}


float sampleShoreWave(float pos)
{
  const float peak_pos = 0.05;

  if (pos < peak_pos)
  {
    pos = pos /= peak_pos;
    return pow(pos, 2);
  }
  else
  {
    pos -= peak_pos;
    pos /= 1.0 - peak_pos;
    return pow(1 - pos, 8);
  }
}


ImageGreyScale::Ptr createShoreWaveTexture()
{
  auto shore_wave = engine::image::create<unsigned char>(0, glm::ivec2(4096, 1));
  assert(shore_wave);
  for (int i = 0; i < shore_wave->w(); i++)
  {
    float pos = 1.0 * i;
    pos /= shore_wave->w();
    float value = sampleShoreWave(pos);
    shore_wave->at(i,0) = value * 255;
  }
  return shore_wave;
}


void resampleTextures(const std::vector<ImageRGBA::ConstPtr> &textures,
                        std::vector<ImageRGBA::ConstPtr> &textures_resampled)
{
  for (ImageRGBA::ConstPtr texture : textures)
  {
    if (texture->w() < texture_size)
      texture = upSample(texture, texture_size / texture->w());
    else if (texture->w() > texture_size)
      texture = downSample(texture, texture->w() / texture_size);
    assert(texture->size() == glm::ivec2(texture_size));

    textures_resampled.push_back(texture);
  }
}


TexturePtr createTextureArray(const std::vector<ImageRGBA::ConstPtr> &textures)
{
  std::vector<ImageRGBA::ConstPtr> textures_resampled;
  resampleTextures(textures, textures_resampled);
  return engine::createTextureArray<ImageRGBA>(textures_resampled);
}


} // namespace



struct engine::MapTextures::Private
{
  shared_ptr<Material> m_material;
  glm::vec3 water_color = glm::vec3(0);
  glm::ivec2 type_map_size = glm::ivec2(0);
  glm::ivec2 water_map_table_size = glm::ivec2(0);
};


engine::MapTextures::MapTextures(TextureManager &texture_manager) :
  m_texture_manager(texture_manager), p(new Private)
{
  p->m_material.reset(new Material(m_texture_manager));
}


engine::MapTextures::~MapTextures()
{
  delete p;
}


void engine::MapTextures::bind()
{
  //HACK
  setTexture(TEXUNIT_SHORE_WAVE, createShoreWaveTexture());

  //HACK
  p->m_material->setTexture(TEXUNIT_GENERIC_NOISE, createNoiseTexture());

  p->m_material->bind();

  CHECK_GL_ERROR();
}


void engine::MapTextures::setUniforms(ShaderProgramPtr program)
{
  using namespace glm;

  assert(p->type_map_size != glm::ivec2(0));

//   vec4 fc = mix(p->forest_color, forest_color, forest_color.a);
//   fc.a = glm::max(p->forest_color.a, forest_color.a);
// //   program->setUniform("forest_color", p->forest_color);
//   program->setUniform("forest_color", fc);
  
//   cout<<"type_map_size: "<<p->type_map_size.x<<","<<p->type_map_size.y<<endl;

  program->setUniform("water_color", p->water_color);
  program->setUniform("typeMapSize", p->type_map_size);
  program->setUniform("water_map_shift", glm::vec2(water_map_shift, water_map_shift));
  program->setUniform("water_map_scale", glm::vec2(1.0 / water_map_scale));
  program->setUniform("water_map_table_size", p->water_map_table_size);

  p->m_material->setUniforms(program);
}


// void engine::MapTextures::setNormalMaps(const std::vector<ImageRGBA::ConstPtr> &textures)
// {
//   assert (!m_normal_maps_id);
//   m_normal_maps_id  = ::createTextureArray(textures);
// }
// 


void engine::MapTextures::setTextures(const std::vector<ImageRGBA::ConstPtr> &textures,
                                      const std::vector<float> &texture_scale)
{
  CHECK_GL_ERROR();

  p->m_material->setTexture(TEXUNIT_TERRAIN, ::createTextureArray(textures));
  CHECK_GL_ERROR();

  p->m_material->setTexture(TEXUNIT_TERRAIN_SCALE_MAP,
                            createFloatTexture1D(texture_scale.data(), texture_scale.size(), 1));
  CHECK_GL_ERROR();
}


void engine::MapTextures::setWaterMap(const std::vector<ImageGreyScale::ConstPtr> &chunks,
                                      Image<unsigned int>::ConstPtr table)
{
  p->water_map_table_size = table->size();

  Image<float>::Ptr table_float =
    image::convert<float>(table);

  TexturePtr chunks_texture = createTextureArray<ImageGreyScale>(chunks);

  TextureParameters<int> chunks_params;
  chunks_params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  chunks_params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  chunks_params.apply(chunks_texture);

  p->m_material->setTexture(TEXUNIT_WATER_MAP, chunks_texture);


  TexturePtr table_texture = createFloatTexture(
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

  p->m_material->setTexture(TEXUNIT_WATER_MAP_TABLE, table_texture);
}


void engine::MapTextures::setTypeMap(ImageGreyScale::ConstPtr type_map)
{
  p->type_map_size = type_map->size();
  TexturePtr t = createTexture<ImageGreyScale>(type_map, false);
//   TexturePtr t = createTexture<ImageGreyScale>(type_map, true);
  TextureParameters<int> params;
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  params.set(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  params.apply(t);
  p->m_material->setTexture(TEXUNIT_TYPE_MAP, t);
}


void engine::MapTextures::setWaterTypeMap(ImageGreyScale::ConstPtr map)
{
  TexturePtr t = createTexture<ImageGreyScale>(map, false);
  TextureParameters<int> params;
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  params.apply(t);
  p->m_material->setTexture(TEXUNIT_WATER_TYPE_MAP, t);
}


void engine::MapTextures::setBeach(std::vector<ImageRGBA::ConstPtr> textures)
{
  p->m_material->setTexture(TEXUNIT_BEACH, ::createTextureArray(textures));
}


void engine::MapTextures::setWaterColor(const glm::vec3 &color)
{
  p->water_color = color;
}


void engine::MapTextures::setForestMap(ImageGreyScale::ConstPtr image)
{
  auto texture = engine::createTexture<ImageGreyScale>(image, true);

  TextureParameters<int> params;

  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//   params.set(GL_TEXTURE_LOD_BIAS, 10.0);

  params.apply(texture);
  
  p->m_material->setTexture(TEXUNIT_FOREST_MAP, texture);
}


void engine::MapTextures::setForestLayers(const std::vector<ImageRGBA::ConstPtr> &images)
{
  TexturePtr texture = createTextureArray<ImageRGBA>(images);

  TextureParameters<int> params;
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//   params.set(GL_TEXTURE_LOD_BIAS, 2.0);
  params.apply(texture);

  p->m_material->setTexture(TEXUNIT_FOREST_LAYERS, texture);
}


void engine::MapTextures::setTexture(unsigned texunit, TexturePtr texture)
{
  TextureParameters<int> params;
  
  switch (texunit)
  {
    case TEXUNIT_TERRAIN_FAR:
      params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      params.set(GL_TEXTURE_LOD_BIAS, 1.0);
      break;
  }
  
  params.apply(texture);

  p->m_material->setTexture(texunit, texture);
}
