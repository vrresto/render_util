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

#include "testbed.h"
#include "scene.h"
#include "camera.h"
#include "map_loader.h"
#include "map_loader_dump.h"
#include "texture.h"
#include <engine.h>
#include <engine/map.h>
#include <engine/map_textures.h>
#include <engine/terrain.h>
#include <engine/terrain_cdlod.h>
#include <engine/texture_util.h>
#include <engine/texunits.h>
#include <engine/image_loader.h>
// #include <engine/image.h>
#include <engine/camera.h>
#include <gl_wrapper/gl_wrapper.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <memory>

#include <gl_wrapper/gl_interface.h>
#include <gl_wrapper/gl_functions.h>
// #include <engine/texunits.h>
#include "shader.h"
#include <engine/shader.h>

using namespace glm;
using namespace std;
using namespace gl_wrapper::gl_functions;
using engine::ShaderProgram;

namespace
{

float elevation_table[256] = {
  #include "height_table"
};

const string shader_path = "shaders/";

engine::ShaderProgramPtr terrain_program;
  
engine::ShaderProgramPtr getTerrainProgram(const engine::TextureManager &tex_mgr)
{
  if (!terrain_program)
  {
    CHECK_GL_ERROR();

    terrain_program.reset(new engine::ShaderProgram("terrain_simple", shader_path));
    terrain_program->error_fail = true;
    terrain_program->setUniformi("sampler_normal_map", 0);
    
    terrain_program->error_fail = false;

    CHECK_GL_ERROR();
  }

  return terrain_program;
}

} // namespace

void destroyShaders()
{
  terrain_program.reset();
}

class ViewHeightmapScene : public Scene
{
  shared_ptr<engine::TerrainBase> m_terrain;
  unsigned int normal_map_id = 0;

public:
  void setup() override;
  void render() override;
};

void ViewHeightmapScene::setup()
{
//   string normal_map_file_path = "../nm.tga";
  string normal_map_file_path = "../LowLand3_normal.tga";
  
  engine::ImageRGBA::Ptr normal_map;
  normal_map.reset(engine::loadImageFromFile<engine::ImageRGBA>(normal_map_file_path));
  assert(normal_map);

  normal_map_id = engine::createTexture<engine::ImageRGBA>(normal_map, false);
  assert(normal_map_id);
  gl::ActiveTexture(GL_TEXTURE0);
  gl::BindTexture(GL_TEXTURE_2D, normal_map_id);
  gl::GenerateMipmap(GL_TEXTURE_2D);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  CHECK_GL_ERROR();

  
  string file_path= "../HeightMap.tga";
//   string file_path= "../LowLand3_bump.tga";

  auto height_map = engine::loadImageFromFile<engine::ImageGreyScale>(file_path);
  assert(height_map);

  vector<float> elevation_map_data(height_map->w() * height_map->h());
  for (unsigned i = 0; i < elevation_map_data.size(); i++)
  {
//     float pixel = height_map->data()[i];
//     float elevation = pixel / 255;
//     elevation *= 1000;

    int index = height_map->data()[i];
    float elevation = elevation_table[index];

    elevation_map_data[i] = elevation;
  }
  engine::ElevationMap elevation_map(height_map->w(), height_map->h(), elevation_map_data);

  m_terrain.reset(new engine::Terrain);
  m_terrain->setTextureManager(&getTextureManager());
  m_terrain->build(&elevation_map);

  vector<vec3> normals = m_terrain->getNormals();
  assert(!normals.empty());

  vector<engine::RGBA> normals_rgba;
  for (vec3 v : normals)
  {
    engine::RGBA rgba =
    { 
      (unsigned char)(v.x * 255),
      (unsigned char)(v.y * 255),
      (unsigned char)(v.z * 255),
      255
    };
    normals_rgba.push_back(rgba);
  }

  engine::ImageRGBA::Ptr img(new engine::ImageRGBA(elevation_map.getWidth(), elevation_map.getHeight(),
                                                   normals_rgba.size() * sizeof(engine::RGBA),
                                                   (const unsigned char *)normals_rgba.data()));
  
//   engine::saveImageToFile("../nm.tga", img.get());
}

void ViewHeightmapScene::render()
{
//   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//   glDisable(GL_DEPTH_TEST);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  m_terrain->update(camera.getPos());

  updateUniforms(getTerrainProgram(getTextureManager()));
  glUseProgram(getTerrainProgram(getTextureManager())->getId());

  m_terrain->draw(getTerrainProgram(getTextureManager()));
}


namespace engine
{
  const std::string &getResourcePath()
  {
    static string path = ".";
    return path;
  }
}

int main()
{
  runApplication<ViewHeightmapScene>();
}
