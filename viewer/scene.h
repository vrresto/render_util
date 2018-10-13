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

#ifndef SCENE_H
#define SCENE_H

#include "camera.h"

#include <render_util/shader.h>
#include <render_util/terrain_util.h>
#include <render_util/globals.h>
#include <gl_wrapper/gl_functions.h>


namespace render_util::viewer
{

using namespace gl_wrapper::gl_functions;


struct Terrain : public render_util::TerrainRenderer
{
  Terrain() {}
  Terrain(const render_util::TerrainRenderer &other)
  {
    *static_cast<render_util::TerrainRenderer*>(this) = other;
  }

  void draw(const Camera &camera)
  {
    gl::FrontFace(GL_CCW);
    gl::Enable(GL_DEPTH_TEST);
    gl::DepthMask(GL_TRUE);
//     gl::PolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    getTerrain()->setDrawDistance(0);
    getTerrain()->update(camera);

    render_util::getCurrentGLContext()->setCurrentProgram(getProgram());

    getTerrain()->draw();

//     gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    CHECK_GL_ERROR();
  }
};


inline Terrain createTerrain(render_util::TextureManager &tex_mgr,
                      bool use_lod,
                      const render_util::ElevationMap::ConstPtr elevation_map,
                      glm::vec3 color,
                      bool use_base_map,
                      bool use_base_water_map)
{
  Terrain t = render_util::createTerrainRenderer(tex_mgr,
    use_lod,
    RENDER_UTIL_SHADER_DIR, "terrain",
    use_base_map,
    use_base_water_map,
    true);

  t.getTerrain()->build(elevation_map);

  t.getProgram()->setUniform("terrain_color", color);
  t.getProgram()->setUniform("draw_near_forest", true);
  t.getProgram()->setUniform("enable_waves", true);
  t.getProgram()->setUniform("enable_terrain_noise", true);

  return t;
}


class Scene
{
  render_util::TextureManager texture_manager = render_util::TextureManager(0);

public:
  virtual ~Scene() {}

  const bool m_use_base_map = true;
  const bool m_use_base_water_map = true;

  Camera camera;
  float sun_azimuth = 90.0;
  bool toggle_lod_morph = false;
  bool pause_animations = false;

  glm::vec2 base_map_origin = glm::vec2(0);
  glm::vec2 base_map_size_m = glm::vec2(0);

  Terrain m_terrain;
  Terrain m_terrain_cdlod;

  glm::vec3 getSunDir()
  {
    glm::vec2 sun_dir_h = glm::vec2(glm::cos(glm::radians(sun_azimuth)), glm::sin(glm::radians(sun_azimuth)));

    return glm::vec3(0, sun_dir_h.x, sun_dir_h.y);
  }

  void createTerrain(const render_util::ElevationMap::ConstPtr elevation_map,
                     const render_util::ElevationMap::ConstPtr elevation_map_base = {})
  {
//     m_terrain =
//       render_util::viewer::createTerrain(getTextureManager(), false,
//                                          elevation_map, glm::vec3(1,0,0));
    m_terrain_cdlod =
      render_util::viewer::createTerrain(getTextureManager(), true,
                                         elevation_map,
                                         glm::vec3(0,1,0),
                                         m_use_base_map,
                                         m_use_base_water_map);
  }

  void updateTerrain(const render_util::ElevationMap::ConstPtr elevation_map_base)
  {
//     m_terrain.getTerrain()->setBaseElevationMap(elevation_map_base);
    m_terrain_cdlod.getTerrain()->setBaseElevationMap(elevation_map_base);
  }

//   void updateTerrain()
//   {
//     m_terrain.update(camera);
//     m_terrain_cdlod.update(camera);
//   }

  void drawTerrain()
  {
//     updateUniforms(m_terrain.getProgram());
//     m_terrain.draw(camera);

    updateUniforms(m_terrain_cdlod.getProgram());
    m_terrain_cdlod.draw(camera);
  }

  render_util::TextureManager &getTextureManager() { return texture_manager; }

  virtual void updateUniforms(render_util::ShaderProgramPtr program)
  {
    program->setUniform("cameraPosWorld", camera.getPos());
    program->setUniform("projectionMatrixFar", camera.getProjectionMatrixFar());
    program->setUniform("world2ViewMatrix", camera.getWorld2ViewMatrix());
    program->setUniform("view2WorldMatrix", camera.getView2WorldMatrix());
    program->setUniform("sunDir", getSunDir());
    program->setUniform("toggle_lod_morph", toggle_lod_morph);
    program->setUniform("height_map_base_origin", base_map_origin);
  }

  virtual void mark() {}
  virtual void unmark() {}
  virtual void cursorPos(const glm::dvec2&) {}
  virtual void rebuild() {}

  virtual void setup() = 0;
  virtual void render(float frame_delta) = 0;
};


} // namespace render_util::viewer


#endif
