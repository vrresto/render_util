/**
 *    Rendering utilities
 *    Copyright (C) 2019 Jan Lepper
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

#include "simple_viewer_application.h"
#include "scene_base.h"
#include "camera.h"
#include <render_util/viewer.h>
#include <render_util/render_util.h>
#include <render_util/water.h>
#include <render_util/shader.h>
#include <render_util/shader_util.h>
#include <render_util/texture_manager.h>
#include <render_util/map.h>
#include <render_util/map_textures.h>
#include <render_util/texture_util.h>
#include <render_util/texunits.h>
#include <render_util/image_loader.h>
#include <render_util/image_util.h>
#include <render_util/elevation_map.h>
#include <render_util/camera.h>
#include <render_util/terrain_util.h>
#include <render_util/image_util.h>
#include <render_util/state.h>
#include <render_util/gl_binding/gl_binding.h>
#include <log.h>

#include <memory>


using namespace render_util::gl_binding;


namespace
{


class SimpleViewerScene : public render_util::viewer::SceneBase
{
  glm::vec2 m_map_size = glm::vec2(0);
  std::shared_ptr<render_util::ElevationMapLoaderBase> m_loader;
  std::shared_ptr<render_util::TerrainBase> m_terrain;

  void createTerrain(render_util::ElevationMap::ConstPtr elevation_map,
                     const render_util::ShaderSearchPath &shader_search_path,
                     const render_util::ShaderParameters &shader_params);
  void drawTerrain();

public:
  SimpleViewerScene(render_util::viewer::CreateElevationMapLoaderFunc&);
  ~SimpleViewerScene() override {}

  void render(float frame_delta) override;
  void setup() override;
  void updateUniforms(render_util::ShaderProgramPtr) override;
};


SimpleViewerScene::SimpleViewerScene(render_util::viewer::CreateElevationMapLoaderFunc &create_loader)
{
  m_loader = create_loader();
}


void SimpleViewerScene::createTerrain(render_util::ElevationMap::ConstPtr elevation_map,
                                      const render_util::ShaderSearchPath &shader_search_path,
                                      const render_util::ShaderParameters &shader_params)
{
  m_terrain = render_util::createTerrain(getTextureManager(), true, shader_search_path);

  m_terrain->setProgramName("terrain_heightmap_only");

  auto material_map = std::make_shared<render_util::TerrainBase::MaterialMap>(elevation_map->getSize());
  render_util::image::fill(material_map, render_util::TerrainBase::MaterialID::LAND);

  auto type_map = std::make_shared<render_util::TerrainBase::TypeMap>(elevation_map->getSize());
  render_util::image::fill(type_map, 0);

  std::vector<render_util::ImageRGBA::Ptr> textures;
  std::vector<render_util::ImageRGB::Ptr> textures_nm;
  const std::vector<float> texture_scale;

  render_util::TerrainBase::BuildParameters params =
  {
    .map = elevation_map,
    .material_map = material_map,
    .type_map = type_map,
    .textures = textures,
    .textures_nm = textures_nm,
    .texture_scale = texture_scale,
    .shader_parameters = shader_params,
  };

  m_terrain->build(params);
}


void SimpleViewerScene::drawTerrain()
{
  gl::FrontFace(GL_CCW);
  gl::Enable(GL_DEPTH_TEST);
  gl::DepthMask(GL_TRUE);

  TerrainClient client(*this);

  m_terrain->setDrawDistance(0);
  m_terrain->update(camera, false);

  m_terrain->draw(&client);
}


void SimpleViewerScene::setup()
{
  getTextureManager().setActive(true);

  render_util::ShaderSearchPath shader_search_path;
  shader_search_path.push_back(RENDER_UTIL_SHADER_DIR);

  auto elevation_map = m_loader->createElevationMap();

  render_util::ShaderParameters shader_params;
  shader_params.set("enable_curvature", false);

  createTerrain(elevation_map, shader_search_path, shader_params);

  m_map_size = glm::vec2(elevation_map->getSize() * m_loader->getMetersPerPixel());
  camera.x = m_map_size.x / 2;
  camera.y = m_map_size.y / 2;
  camera.z = 10000;
}


void SimpleViewerScene::render(float frame_delta)
{
  drawTerrain();
}

void SimpleViewerScene::updateUniforms(render_util::ShaderProgramPtr program)
{
  SceneBase::updateUniforms(program);
  program->setUniform("map_size", m_map_size);
  program->setUniformi("map_resolution_m", m_loader->getMetersPerPixel());
}


} // namespace


void render_util::viewer::runSimpleViewer(CreateElevationMapLoaderFunc &create_map_loader, std::string app_name)
{
  auto create_func = [&create_map_loader]
  {
    auto scene = std::make_shared<SimpleViewerScene>(create_map_loader);
    return scene;
  };

  runSimpleApplication(create_func, app_name);

  LOG_INFO<<"exiting..."<<std::endl;
}
