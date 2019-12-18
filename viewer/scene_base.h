/**
 *    Rendering utilities
 *    Copyright (C) 2019  Jan Lepper
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

#ifndef RENDER_UTIL_VIEWER_SCENEBASE_H
#define RENDER_UTIL_VIEWER_SCENEBASE_H

#include "camera.h"

#include <render_util/render_util.h>
#include <render_util/map_loader_base.h>
#include <render_util/shader.h>
#include <render_util/terrain_util.h>
#include <render_util/atmosphere.h>
#include <render_util/globals.h>
#include <render_util/parameter_wrapper.h>
#include <render_util/gl_binding/gl_functions.h>


namespace render_util::viewer
{


class SceneBase
{
public:
  struct ParameterBase
  {
    const std::string name;

    ParameterBase(std::string name) : name(name) {}

    virtual void reset() = 0;
    virtual void changeX(float step_factor) = 0;
    virtual void changeY(float step_factor) {}
    virtual void changeZ(float step_factor) {}
    virtual std::string getValueString() = 0;
  };

  struct ParameterSimple : public ParameterBase
  {
    const float default_value = 0;
    const float step = 1;
    float &value;

    ParameterSimple(std::string name, float &value, float step) : ParameterBase(name),
      default_value(value), step(step), value(value)
    {
    }

    void reset() override
    {
      value = default_value;
    }

    void changeX(float step_factor) override
    {
      value += step_factor * step;
    }

    std::string getValueString() override
    {
      char buf[100];
      snprintf(buf, sizeof(buf), "%.2f", value);
      return buf;
    }
  };

  struct ParameterVec2 : public ParameterBase
  {
    const glm::vec2 default_value = glm::vec2(0);
    const float step = 1;
    glm::vec2 &value;

    ParameterVec2(std::string name, glm::vec2 &value, float step) : ParameterBase(name),
      default_value(value), step(step), value(value)
    {
    }

    void reset() override
    {
      value = default_value;
    }

    void changeX(float step_factor) override
    {
      value.x += step_factor * step;
    }

    void changeY(float step_factor) override
    {
      value.y += step_factor * step;
    }

    std::string getValueString() override
    {
      char buf[100];
      snprintf(buf, sizeof(buf), "[%.2f, %.2f]", value.x, value.y);
      return buf;
    }
  };

  class TerrainClient : public TerrainBase::Client
  {
    SceneBase &m_scene;

  public:
    TerrainClient(SceneBase &scene) : m_scene(scene) {}

    void setActiveProgram(ShaderProgramPtr p) override
    {
      render_util::getCurrentGLContext()->setCurrentProgram(p);
      m_scene.updateUniforms(p);
    }
  };

  Camera camera;

  virtual ~SceneBase() {}

  const std::vector<std::unique_ptr<ParameterBase>> &getParameters() { return m_parameters; }
  int getActiveParameterIndex() { return m_active_parameter_index; }
  void setActiveParameter(int index)
  {
    if (m_parameters.empty())
      return;

    if (index < 0)
      index += m_parameters.size();
    index = index % m_parameters.size();
    m_active_parameter_index = index;
  }
  ParameterBase &getActiveParameter() { return *m_parameters.at(m_active_parameter_index); }

  render_util::TextureManager &getTextureManager() { return m_texture_manager; }

  virtual glm::vec3 getSunDir() { return glm::normalize(glm::vec3(0.5)); }

  virtual void updateUniforms(render_util::ShaderProgramPtr program)
  {
    render_util::updateUniforms(program, camera);
    program->setUniform("sunDir", getSunDir());
    program->setUniform("terrain_height_offset", 0.f);
  }

  virtual void cursorPos(const glm::dvec2&) {}

  virtual void setup() = 0;
  virtual void render(float frame_delta) = 0;
  virtual void save() = 0;

protected:

  void addParameter(std::string name, float &value, float step)
  {
    auto p = std::make_unique<ParameterSimple >(name, value, step);
    m_parameters.push_back(std::move(p));
  }

  void addParameter(std::string name, glm::vec2 &value, float step)
  {
    auto p = std::make_unique<ParameterVec2>(name, value, step);
    m_parameters.push_back(std::move(p));
  }


private:
  render_util::TextureManager m_texture_manager = render_util::TextureManager(0);
  std::vector<std::unique_ptr<ParameterBase>> m_parameters;
  int m_active_parameter_index = 0;
};


} // namespace render_util::viewer


#endif
