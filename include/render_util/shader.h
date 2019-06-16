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

#ifndef RENDER_UTIL_SHADER_H
#define RENDER_UTIL_SHADER_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <cstdio>
#include <cassert>
#include <memory>
#include <glm/glm.hpp>

namespace render_util
{
  using ShaderSearchPath = std::vector<std::string>;

  class ShaderParameters
  {
    std::unordered_map<std::string, std::string> m_map;

  public:
    const std::string &get(const std::string &name) const;
    void set(const std::string &name, const std::string &value);

    template <typename T>
    void set(const std::string &name, const T &value)
    {
      set(name, std::to_string(value));
    }
  };


  class Shader
  {
    unsigned int m_id = 0;
    unsigned int m_type = 0;
    std::string m_preprocessed_source;
    std::string m_name;
    std::string m_filename;
    std::vector<std::string> m_includes;

  public:
    Shader(const std::string &name,
                     const std::vector<std::string> &paths,
                     unsigned int type,
                     const ShaderParameters &params);

    ~Shader();

    void preProcess(const std::vector<char> &in, const ShaderParameters &params,
                        const std::vector<std::string> &paths);
    void compile();
    unsigned int getID() { return m_id; }
    const std::string &getName() { return m_name; }
    const std::string &getFileName() { return m_filename; }
  };


  class ShaderProgram
  {
  public:
    unsigned long long frame_nr = 0;
    bool is_far_camera = false;

    ShaderProgram() {}
    ShaderProgram(const std::string &name,
                  const std::vector<std::string> &vertex_shaders,
                  const std::vector<std::string> &fragment_shaders,
                  const std::vector<std::string> &geometry_shaders,
                  const std::vector<std::string> &paths,
                  bool must_be_valid = true,
                  const std::map<unsigned int, std::string> &attribute_locations = {},
                  const ShaderParameters &parameters = {});

    ~ShaderProgram();

    void assertUniformsAreSet();

    int getUniformLocation(const std::string &name);
//     bool isActive();
//     bool isLinked() { return true; } //FIXME
    unsigned int getId();
//     const std::string &getName()
//     {
//       return name;
//     }

    void setUniformi(const std::string &name, const int value)
    {
      int location = getUniformLocation(name);
      if (location != -1)
      {
        setUniformi(location, value);
      }
      else if (error_fail)
      {
        printf("uniform not found: %s - program: %s\n", name.c_str(), this->name.c_str());
        exit(1);
      }
    }


    template <typename T>
    void setUniform(const std::string &name, const T &value)
    {
      int location = getUniformLocation(name);
      if (location != -1)
      {
        setUniform(location, value);
      }
      else if (error_fail)
      {
        printf("uniform not found: %s - program: %s\n", name.c_str(), this->name.c_str());
        exit(1);
      }
    }

    bool error_fail = false;

    bool isValid() { return is_valid; }

  private:
    const ShaderParameters m_parameters = {};

    unsigned int id = 0;
    std::string name;
    std::vector<std::string> paths;

    std::vector<std::string> vertex_shaders;
    std::vector<std::string> fragment_shaders;
    std::vector<std::string> geometry_shaders;

    std::vector<std::unique_ptr<Shader>> shaders;

    const std::map<unsigned int, std::string> attribute_locations;
    std::unordered_map<std::string, int> uniform_locations;
    std::unordered_set<int> set_uniforms;
    bool is_valid = false;
    bool must_be_valid = true;

    void link();
    void create();
    void assertIsValid();
    void setUniformi(int location, int);
    void setUniform(int location, const int&);
    void setUniform(int location, const bool&);
    void setUniform(int location, const float&);
    void setUniform(int location, const glm::vec2&);
    void setUniform(int location, const glm::vec3&);
    void setUniform(int location, const glm::vec4&);
    void setUniform(int location, const glm::ivec2&);
    void setUniform(int location, const glm::mat4&);
  };

  typedef std::shared_ptr<ShaderProgram> ShaderProgramPtr;
}

#endif
