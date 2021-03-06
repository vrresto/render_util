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

#include <render_util/image_resample.h>
#include <render_util/image_loader.h>
#include <render_util/image_util.h>
#include <render_util/texture_util.h>
#include <render_util/texunits.h>
#include <render_util/elevation_map.h>

#include <fstream>
#include <memory>
#include <iostream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/gl.h>

#include <render_util/gl_binding/gl_functions.h>
#include <atmosphere_map.h>
#include <curvature_map.h>
#include <log.h>

using namespace render_util::gl_binding;
using namespace render_util;
using namespace glm;
using namespace std;

namespace
{


void createTextureArrayLevel0(const std::vector<const unsigned char*> &textures,
                              int texture_width,
                              int bytes_per_pixel)
{
  using namespace std;
  using std::min;

  CHECK_GL_ERROR();

  size_t max_levels = 0;
  gl::GetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, (int*) &max_levels);
  LOG_TRACE<<"max_levels: "<<max_levels<<endl;

  const unsigned texture_size = texture_width * texture_width * bytes_per_pixel;
  const size_t num_textures = min(textures.size(), max_levels);
  const float array_size_mb = num_textures * texture_size / 1024.f / 1024.f;

  LOG_TRACE<<"num_textures: "<<num_textures<<endl;
  LOG_TRACE<<"texture_size: "<<texture_size<<endl;
  LOG_TRACE<<"texture_width: "<<texture_width<<endl;
  LOG_TRACE<<"bytes_per_pixel: "<<bytes_per_pixel<<endl;

  GLint internal_format = -1;
  GLint format = -1;

  switch (bytes_per_pixel)
  {
    case 1:
      internal_format = GL_R8;
      format = GL_RED;
      break;
    case 3:
      internal_format = GL_RGB8;
      format = GL_RGB;
      break;
    case 4:
      internal_format = GL_RGBA8;
      format = GL_RGBA;
      break;
    default:
      assert(0);
      abort();
  }

  LOG_TRACE<<"reserving gl memory (" << array_size_mb << " MB) ..."<<endl;
  gl::TexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format,
            texture_width, texture_width, num_textures,
            0, format, GL_UNSIGNED_BYTE, nullptr);
  FORCE_CHECK_GL_ERROR();
  LOG_TRACE<<"reserving gl memory ... done"<<endl;

  for (unsigned i = 0; i < num_textures; i++)
  {
    auto data = textures.at(i);

//     LOG_TRACE<<"calling gl::TexSubImage3D() ..."<<endl;
    gl::TexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
            0, 0, i,
            texture_width, texture_width, 1,
            format, GL_UNSIGNED_BYTE, data);
    FORCE_CHECK_GL_ERROR();
//     LOG_TRACE<<"calling gl::TexSubImage3D() ... done"<<endl;
  }

  FORCE_CHECK_GL_ERROR();
}


struct NormalMapCreator
{
  render_util::ElevationMap::ConstPtr elevation_map;
  render_util::Image<Normal>::Ptr normals;
  float grid_scale = 1.0;

  vec3 getNormal(ivec2 coords) const
  {
    return normals->get(coords).to_vec3();
  }

  void setNormal(ivec2 coords, glm::vec3 normal)
  {
    normals->at(coords) = { normal.x, normal.y, normal.z };
  }

  glm::vec3 calcTriangleNormal(const vec2 vertex_coords[3])
  {
    vec3 vertices[3];
    for (unsigned int i = 0; i < 3; i++)
      vertices[i] = getVertex(vertex_coords[i]);

    return calcNormal(vertices);
  }

  vec3 getVertex(ivec2 coords)
  {
    vec3 v;
    v.x = coords.x * grid_scale;
    v.y = coords.y * grid_scale;
    v.z = elevation_map->get(coords.x, coords.y);
    return v;
  }

  void calcNormals()
  {
    normals.reset(new Image<Normal>(elevation_map->getSize()));

    for (int y = 0; y < elevation_map->getHeight()-1; y++)
    {
      for (int x = 0; x < elevation_map->getWidth()-1; x++)
      {
        vec2 triangle0[3] =
        {
          vec2(x+1, y+1),
          vec2(x+0, y+0),
          vec2(x+1, y+0)
        };

        vec2 triangle1[3] =
        {
          vec2(x+1, y+1),
          vec2(x+0, y+1),
          vec2(x+0, y+0)
        };

        vec3 normal0 = calcTriangleNormal(triangle0);
        for (unsigned int i = 0; i < 3; i++)
          setNormal(triangle0[i], glm::normalize(getNormal(triangle0[i]) + normal0));

        vec3 normal1 = calcTriangleNormal(triangle1);
        for (unsigned int i = 0; i < 3; i++)
          setNormal(triangle1[i], glm::normalize(getNormal(triangle1[i]) + normal1));
      }
    }

  }

};

float mapFloatToUnsignedChar(float value)
{
  return ((value + 1.0) / 2.0) * 255;
}

// ImageRGBA::Ptr createNormalMapFromElevationMap(ElevationMap *elevation_map)
// {
//   NormalMapCreator c;
//   c.elevation_map = *elevation_map;
//   c.calcNormals();
// 
//   ImageRGBA::Ptr image(new ImageRGBA(elevation_map->getSize()));
//   for (int y = 0; y < image->h(); y++)
//   {
//     for (int x = 0; x < image->w(); x++)
//     {
//       const Normal &n = c.normals->getPixel(x, y);
// 
//       assert(n.x <= 1);
//       assert(n.y <= 1);
//       assert(n.z >= 0);
// 
//       RGBA pixel;
//       pixel.r = mapFloatToUnsignedChar(n.x);
//       pixel.g = mapFloatToUnsignedChar(n.y);
//       pixel.b = mapFloatToUnsignedChar(n.z);
//       
// //       LOG_TRACE<<"g: "<<(int)pixel.g<<endl;
// //       assert(pixel.g <= 55);
//       
//       pixel.a = 255;
//       image->setPixel(x, y, pixel);
//     }
//   }
//   return image;
// }


} // namespace

namespace render_util
{


void setTextureImage(TexturePtr texture,
                     const unsigned char *data,
                     int w,
                     int h,
                     int bytes_per_pixel,
                     bool mipmaps)
{
  assert(data);
  assert(w);
  assert(h);
  CHECK_GL_ERROR();

  TemporaryTextureBinding binding(texture);
  CHECK_GL_ERROR();

  GLint internal_format = -1;
  GLint format = -1;

  switch (bytes_per_pixel)
  {
    case 1:
      internal_format = GL_R8;
      format = GL_RED;
      break;
    case 2:
      internal_format = GL_RG8;
      format = GL_RG;
      break;
    case 3:
      internal_format = GL_RGB8;
      format = GL_RGB;
      break;
    case 4:
      internal_format = GL_RGBA8;
      format = GL_RGBA;
      break;
    default:
      LOG_TRACE<<bytes_per_pixel<<endl;
      assert(0);
      abort();
  }

  gl::TexImage2D(GL_TEXTURE_2D, 0,
                internal_format,
                w,
                h,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data);

  CHECK_GL_ERROR();

  if (mipmaps)
  {
    gl::GenerateMipmap(GL_TEXTURE_2D);
//     gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  }
//   else
//     gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  CHECK_GL_ERROR();
}


TexturePtr createTexture(const unsigned char *data, int w, int h, int bytes_per_pixel, bool mipmaps)
{
  assert(data);
  assert(w);
  assert(h);

  CHECK_GL_ERROR();

  TexturePtr texture = Texture::create(GL_TEXTURE_2D);
  CHECK_GL_ERROR();

  TemporaryTextureBinding binding(texture);
  CHECK_GL_ERROR();

  GLint internal_format = -1;
  GLint format = -1;

  switch (bytes_per_pixel)
  {
    case 1:
      internal_format = GL_R8;
      format = GL_RED;
      break;
    case 2:
      internal_format = GL_RG8;
      format = GL_RG;
      break;
    case 3:
      internal_format = GL_RGB8;
      format = GL_RGB;
      break;
    case 4:
      internal_format = GL_RGBA8;
      format = GL_RGBA;
      break;
    default:
      LOG_TRACE<<bytes_per_pixel<<endl;
      assert(0);
      abort();
  }

  gl::TexImage2D(GL_TEXTURE_2D, 0,
                internal_format,
                w,
                h,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data);

  CHECK_GL_ERROR();

  if (mipmaps)
  {
    gl::GenerateMipmap(GL_TEXTURE_2D);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  }
  else
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  
  CHECK_GL_ERROR();

  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  return texture;
}


TexturePtr createTextureExt(const unsigned char *data,
                          size_t data_size,
                          int w, int h,
                          int num_components,
                          size_t bytes_per_component,
                          int type,
                          int format,
                          int internal_format,
                          bool mipmaps)
{
  assert(data);
  assert(w);
  assert(h);
  assert(data_size == (w * h * num_components * bytes_per_component));

  FORCE_CHECK_GL_ERROR();

  TexturePtr texture = Texture::create(GL_TEXTURE_2D);
  CHECK_GL_ERROR();

  TemporaryTextureBinding binding(texture);
  CHECK_GL_ERROR();

  LOG_TRACE << "reserving texture memory: " << w << "x" << h
    << " (" << num_components << " components, " << bytes_per_component << " bytes per component, "
    << data_size/1024.0/1024.0 << " mb)"
    << endl;

  gl::TexImage2D(GL_TEXTURE_2D, 0,
                internal_format,
                w,
                h,
                0,
                format,
                type,
                data);

  FORCE_CHECK_GL_ERROR();

  if (mipmaps)
  {
    gl::GenerateMipmap(GL_TEXTURE_2D);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  }
  else
  {
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }

  FORCE_CHECK_GL_ERROR();

  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  return texture;
}


TexturePtr createTextureArray(const std::vector<const unsigned char*> &textures,
                                int mipmap_levels, int texture_width, int bytes_per_pixel)
{
  assert(!textures.empty());

  CHECK_GL_ERROR();

  TexturePtr texture = Texture::create(GL_TEXTURE_2D_ARRAY);
  TemporaryTextureBinding binding(texture);

  CHECK_GL_ERROR();

  gl::TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  createTextureArrayLevel0(textures, texture_width, bytes_per_pixel);
  
  CHECK_GL_ERROR();

  gl::GenerateMipmap(GL_TEXTURE_2D_ARRAY);

  CHECK_GL_ERROR();

  return texture;
}


TexturePtr createFloatTexture1D(const float *data, size_t size, int num_components)
{
  TexturePtr texture = Texture::create(GL_TEXTURE_1D);
  TemporaryTextureBinding binding(texture);

  gl::TexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  gl::TexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  gl::TexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  gl::TexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  GLint internal_format = -1;
  GLint format = -1;

  switch (num_components)
  {
    case 1:
      internal_format = GL_R32F;
      format = GL_RED;
      break;
    case 2:
      internal_format = GL_RG32F;
      format = GL_RG;
      break;
    case 3:
      internal_format = GL_RGB32F;
      format = GL_RGB;
      break;
    default:
      assert(0);
      abort();
  }
  
  gl::TexImage1D(GL_TEXTURE_1D,
                0,
                internal_format,
                size,
                0,
                format,
                GL_FLOAT,
                data);

  CHECK_GL_ERROR();

  return texture;
}

TexturePtr createFloatTexture(const float *data, int w, int h, int num_components, bool mipmaps)
{
  CHECK_GL_ERROR();

  TexturePtr texture = Texture::create(GL_TEXTURE_2D);
  TemporaryTextureBinding binding(texture);

  CHECK_GL_ERROR();
  
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  GLint internal_format = -1;
  GLint format = -1;

  switch (num_components)
  {
    case 1:
      internal_format = GL_R32F;
      format = GL_RED;
      break;
    case 2:
      internal_format = GL_RG32F;
      format = GL_RG;
      break;
    case 3:
      internal_format = GL_RGB32F;
      format = GL_RGB;
      break;
    default:
      assert(0);
      abort();
  }

  gl::TexImage2D(GL_TEXTURE_2D, 0,
                internal_format,
                w,
                h,
                0,
                format,
                GL_FLOAT,
                data);

  if (mipmaps)
    gl::GenerateMipmap(texture->getTarget());

  CHECK_GL_ERROR();

  return texture;
}


#if 0
unsigned int createUnsignedIntTexture(const unsigned int *data, int w, int h)
{
  GLuint current_texture_save;
  gl::GetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*) &current_texture_save);

  CHECK_GL_ERROR();

  GLuint texture = 0;
  gl::GenTextures(1, &texture);

  assert(texture);

  gl::BindTexture(GL_TEXTURE_2D, texture);    

  CHECK_GL_ERROR();

  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//     gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//     gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  GLint internal_format = GL_R32UI;
  GLint format = GL_RED;

  CHECK_GL_ERROR();

  gl::TexImage2D(GL_TEXTURE_2D,
              0,
              internal_format,
              w,
              h,
              0,
              format,
              GL_UNSIGNED_INT,
              data);

  CHECK_GL_ERROR();
  exit(1);
  CHECK_GL_ERROR();

  gl::BindTexture(GL_TEXTURE_2D, current_texture_save);

  return texture;
}
#endif

// namespace
// {
//   using namespace glm;
// 
//   glm::vec3 getNormal(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
//   {
// //       vec3 u = p2 - p1;
// //       vec3 v = p3 - p1;
// // 
// //       vec3 normal;
// // 
// //       normal.x = (u.y * v.z) - (u.z * v.y);
// //       normal.y = (u.z * v.x) - (u.x * v.z);
// //       normal.z = (u.x * v.y) - (u.y * v.x);
// //       return normal;
// 
//     vec3 a,b;
//     a = p1 - p2;
//     b = p1 - p3;
// 
//     return normalize(cross(a,b));
// 
// 
//   }
// 
//   vec3 getTerrainVertex(const render_util::ElevationMap &elevation_map, int x, int y)
//   {
//     const float grid_resolution = 200.0;
//     
//     return vec3(x * grid_resolution, y * grid_resolution, elevation_map->get(x, y));
//   }
// }

// ImageGreyScale::Ptr createTerrainLightMap(const ElevationMap &elevation_map)
// {
//   using namespace glm;
//   
//   vec3 sun_direction(normalize(vec3(0.5, 0.5, 0.7)));
// //     vec3 sun_direction(normalize(vec3(0.0, 0.0, 1.0)));
// 
//   ImageGreyScale *image = new ImageGreyScale(ivec2(elevation_map.getWidth(), elevation_map.getHeight()));
// 
//   for (int y = 0; y < elevation_map.getHeight(); y++)
//   {
//     for (int x = 0; x < elevation_map.getWidth(); x++)
//     {
//       vec3 v00 = getTerrainVertex(elevation_map, x+1, y+1);
//       vec3 v10 = getTerrainVertex(elevation_map, x+0, y+0);
//       vec3 v20 = getTerrainVertex(elevation_map, x+1, y+0);
// 
//       vec3 n0 = getNormal(v00, v10, v20);
// 
//       vec3 v01 = getTerrainVertex(elevation_map, x+1, y+1);
//       vec3 v11 = getTerrainVertex(elevation_map, x+0, y+1);
//       vec3 v21 = getTerrainVertex(elevation_map, x+0, y+0);
// 
//       vec3 n1 = getNormal(v01, v11, v21);
//       
// //         LOG_TRACE<<n1.x<<" "<<n1.y<<" "<<n1.z<<endl;
// 
//       vec3 n = n0 + n1;
//       n *= 0.5;
// 
//       float light = glm::clamp(dot(n, sun_direction), 0.0f, 1.0f);
// //         dot(n, sun_direction);
//       
// //         light = !(x % 2) && (y % 2) ? 1.0 : 0.0;
// 
//       image->setPixel(x, y, light * 255);
//     }
//   }
// 
//   return ImageGreyScale::Ptr(image);
// }


TexturePtr createAmosphereThicknessTexture(TextureManager &texture_manager,
                                      std::string resource_path)
{
  AtmosphereMapElementType *atmosphere_map =
    new AtmosphereMapElementType[atmosphere_map_num_elements];

  ifstream in(resource_path + "/atmosphere_map", ios_base::binary);
  assert(in.good());

  in.read((char*) atmosphere_map, atmosphere_map_size_bytes);
  assert(in.good());

  const glm::ivec2 dimensions = atmosphere_map_dimensions;

  TexturePtr texture = createFloatTexture((float*)atmosphere_map, dimensions.x, dimensions.y, 2);

  TextureParameters<int> params;
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  params.apply(texture);

  texture_manager.bind(TEXUNIT_ATMOSPHERE_THICKNESS_MAP, texture);

  CHECK_GL_ERROR();

  delete[] atmosphere_map;

  return texture;
}

TexturePtr createCurvatureTexture(TextureManager &texture_manager,
                            std::string resource_path)
{
  vector<char> curvature_map;
  if (!util::readFile(resource_path + "/curvature_map", curvature_map))
  {
    assert(0);
  }
  assert(curvature_map.size() == curvature_map_size_bytes);

  TexturePtr texture = createFloatTexture((const float*)curvature_map.data(),
                                          curvature_map_width,
                                          curvature_map_height,
                                          2);

  TextureParameters<int> params;
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  params.apply(texture);

  texture_manager.bind(TEXUNIT_CURVATURE_MAP, texture);

  CHECK_GL_ERROR();

  return texture;
}


ImageRGBA::Ptr createMapFarTexture(ImageGreyScale::ConstPtr type_map,
                            const vector<ImageRGBA::ConstPtr> &textures,
                            int type_map_meters_per_pixel,
                            int meters_per_tile)

{
  //FIXME take texture scale into account

  typedef Sampler<ImageRGBA> SamplerRGBA;
  typedef Surface<ImageRGBA> SurfaceRGBA;

  LOG_DEBUG<<"generating far texture..."<<endl;

  int tile_size_pixels = meters_per_tile / type_map_meters_per_pixel;
  assert(tile_size_pixels * type_map_meters_per_pixel == meters_per_tile);

  ImageRGBA::Ptr texture = make_shared<ImageRGBA>(type_map->size());

  vector<ImageRGBA::Ptr> mipmaps(textures.size());
  for (size_t i = 0; i < mipmaps.size(); i++)
  {
    if (textures[i])
    {
      mipmaps[i] = downSample(textures[i], textures[i]->w() / tile_size_pixels);
      //     stringstream path;
      //     path << "dump/" << i << ".tga";
      //     saveImageToFile(path.str(), mipmaps[i].get());
    }
  }

  for (int y = 0; y < texture->h(); y++)
  {
    for (int x = 0; x < texture->w(); x++)
    {
      for (int i = 0; i < texture->numComponents(); i++)
      {
        int color = 0;

        unsigned type = type_map->get(x,y);

        assert(type < mipmaps.size());

        if (type >= mipmaps.size())
          type = 0;

        auto mipmap = mipmaps[type];
        if (mipmap)
        {
          ivec2 tile_coords = ivec2(x,y);
          tile_coords %= tile_size_pixels;
          color = mipmap->get(tile_coords, i);
        }

        texture->at(x,y,i) = color;
      }
    }
  }

//   exit(0);
  LOG_DEBUG<<"generating map texture finished."<<endl;

  return texture;
}


ImageRGB::Ptr createNormalMap(ImageGreyScale::ConstPtr height_map,
                              float max_height_m,
                              float height_map_width_m)
{
  assert(height_map);
//   auto height_map_upsampled = upSample(height_map, 4);
  auto height_map_upsampled = height_map;
  assert(height_map_upsampled);

  auto elevation_map = make_shared<ElevationMap>(height_map_upsampled->size());

  for (int y = 0; y < elevation_map->h(); y++)
  {
    for (int x = 0; x < elevation_map->w(); x++)
    {
      auto value = height_map_upsampled->get(x,y);
      elevation_map->at(x,y) = (value / 255.f) * max_height_m;
    }
  }

  const float grid_scale = height_map_width_m / height_map->w();

  auto normal_map = createNormalMap(elevation_map, grid_scale);

  auto normal_map_rgb = make_shared<ImageRGB>(normal_map->size());

  for (int y = 0; y < normal_map_rgb->h(); y++)
  {
    for (int x = 0; x < normal_map_rgb->w(); x++)
    {
      auto normal = normal_map->getPixel(x,y)[0].to_vec3();

      for (int comp = 0; comp < normal_map_rgb->numComponents(); comp++)
      {
        normal_map_rgb->at(x,y,comp) = ((normal[comp] + 1.f) / 2.f) * 255.f;
      }
    }
  }

//   normal_map_rgb = downSample(normal_map_rgb, 4);

  return normal_map_rgb;
}


Image<Normal>::Ptr createNormalMap(ElevationMap::ConstPtr elevation_map, float grid_scale)
{
  NormalMapCreator c;
  c.grid_scale = grid_scale;
  c.elevation_map = elevation_map;
  c.calcNormals();

  return c.normals;
}


} // namespace render_util


namespace render_util::texture_format
{


int TextureFormatParametersBase::getFormat(unsigned int num)
{
  switch (num)
  {
    case 1:
      return GL_RED;
    case 2:
      return GL_RG;
    case 3:
      return GL_RGB;
    case 4:
      return GL_RGBA;
    default:
      assert(0);
      abort();
  }
}


int TextureFormatParameters<unsigned char>::getType()
{
   return GL_UNSIGNED_BYTE;
}


int TextureFormatParameters<unsigned char>::getInternalFormat(unsigned int num)
{
  switch (num)
  {
    case 1:
      return GL_R8;
    case 2:
      return GL_RG8;
    case 3:
      return GL_RGB8;
    case 4:
      return GL_RGBA8;
    default:
      assert(0);
      abort();
  }
}


int TextureFormatParameters<float>::getType()
{
  return GL_FLOAT;
}


int TextureFormatParameters<float>::getInternalFormat(unsigned int num)
{
  switch (num)
  {
    case 1:
      return GL_R32F;
    case 2:
      return GL_RG32F;
    case 3:
      return GL_RGB32F;
    case 4:
      return GL_RGBA32F;
    default:
      assert(0);
      abort();
  }
};


int TextureFormatParameters<half_float::half>::getType()
{
  return GL_HALF_FLOAT;
}


int TextureFormatParameters<half_float::half>::getInternalFormat(unsigned int num)
{
  switch (num)
  {
    case 1:
      return GL_R16F;
    case 2:
      return GL_RG16F;
    case 3:
      return GL_RGB16F;
    case 4:
      return GL_RGBA16F;
    default:
      assert(0);
      abort();
  }
};


} // namespace render_util::texture_format
