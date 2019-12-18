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

#define GLM_ENABLE_EXPERIMENTAL

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

#include <glm/gtx/vec_swizzle.hpp>
#include <memory>


using namespace render_util::gl_binding;


namespace
{


class SimpleViewerScene : public render_util::viewer::SceneBase
{
  glm::vec2 m_map_size = glm::vec2(0);
  std::shared_ptr<render_util::ElevationMapLoaderBase> m_loader;
  std::shared_ptr<render_util::TerrainBase> m_terrain;
  float m_base_map_height = 0;
  glm::vec2 m_base_map_origin = glm::vec2(0);

  void createTerrain(render_util::ElevationMap::ConstPtr elevation_map,
                     render_util::ElevationMap::ConstPtr base_elevation_map,
                     unsigned int base_elevation_map_resolution_m,
                     glm::dvec2 base_map_origin,
                     const render_util::ShaderSearchPath &shader_search_path,
                     const render_util::ShaderParameters &shader_params);
  void drawTerrain();

public:
  SimpleViewerScene(render_util::viewer::CreateElevationMapLoaderFunc&);
  ~SimpleViewerScene() override {}

  void render(float frame_delta) override;
  void setup() override;
  void updateUniforms(render_util::ShaderProgramPtr) override;
  void save() override;
};


SimpleViewerScene::SimpleViewerScene(render_util::viewer::CreateElevationMapLoaderFunc &create_loader)
{
  m_loader = create_loader();
}


void SimpleViewerScene::createTerrain(render_util::ElevationMap::ConstPtr elevation_map,
                                      render_util::ElevationMap::ConstPtr base_elevation_map,
                                      unsigned int base_elevation_map_resolution_m,
                                      glm::dvec2 base_map_origin,
                                      const render_util::ShaderSearchPath &shader_search_path,
                                      const render_util::ShaderParameters &shader_params)
{
  m_terrain = render_util::createTerrain(getTextureManager(), true, shader_search_path);

  m_terrain->setProgramName("terrain_heightmap_only");
  m_terrain->setBaseMapOrigin(base_map_origin);

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
    .base_map = base_elevation_map,
    .base_map_resolution_m = base_elevation_map_resolution_m,
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

//   gl::PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  m_terrain->draw(&client);
//   gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


void SimpleViewerScene::setup()
{
  getTextureManager().setActive(true);

  render_util::ShaderSearchPath shader_search_path;
  shader_search_path.push_back(RENDER_UTIL_SHADER_DIR);

  auto elevation_map = m_loader->createElevationMap();
  auto base_elevation_map = m_loader->createBaseElevationMap();

  m_map_size = glm::vec2(elevation_map->getSize() * m_loader->getMetersPerPixel());

  auto map_center = m_map_size / 2.f;

  if (base_elevation_map)
  {
    auto base_map_size =
      m_loader->getBaseElevationMapMetersPerPixel() * base_elevation_map->getSize();

    m_base_map_origin = -1.f * (glm::vec2(base_map_size) / glm::vec2(2));
    m_base_map_origin += map_center;
  }

  {
    auto origin = m_loader->getBaseElevationMapOrigin(glm::vec3(m_base_map_origin, m_base_map_height));
    m_base_map_origin = glm::xy(origin);
    m_base_map_height = origin.z;
  }

  render_util::ShaderParameters shader_params;
  shader_params.set("enable_curvature", false);

  createTerrain(elevation_map,
                base_elevation_map,
                m_loader->getBaseElevationMapMetersPerPixel(),
                m_base_map_origin,
                shader_search_path,
                shader_params);

  camera.x = map_center.x;
  camera.y = map_center.y;
  camera.z = 10000;

  addParameter("base_map_height", m_base_map_height, 100.f);
  addParameter("base_map_origin", m_base_map_origin, 1000.f);
}


void SimpleViewerScene::render(float frame_delta)
{
  m_terrain->setBaseMapOrigin(m_base_map_origin);
  drawTerrain();
}

void SimpleViewerScene::updateUniforms(render_util::ShaderProgramPtr program)
{
  SceneBase::updateUniforms(program);
  program->setUniform("terrain_base_map_height", m_base_map_height);
}


void SimpleViewerScene::save()
{
  m_loader->saveBaseElevationMapOrigin(glm::vec3(m_base_map_origin, m_base_map_height));
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
