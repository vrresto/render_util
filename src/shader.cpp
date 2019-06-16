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

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cstdlib>
#include <glm/gtc/type_ptr.hpp>
#include <GL/gl.h>

#include <render_util/render_util.h>
#include <distances.h>
#include <curvature_map.h>
#include <render_util/shader.h>
#include <render_util/gl_binding/gl_functions.h>

using namespace render_util::gl_binding;
using namespace std;
using namespace glm;
using namespace render_util;

namespace
{


string getParameterValue(const string &parameter, const ShaderParameters &params)
{
  auto pos = parameter.find(':');

  string parameter_name;
  string default_value;

  if (pos != string::npos && pos < parameter.size()-1)
  {
    parameter_name = parameter.substr(0, pos);
    default_value = parameter.substr(pos+1);
  }
  else
  {
    parameter_name = parameter;
  }

  assert(!parameter_name.empty());

  try
  {
    return params.get(parameter_name);
  }
  catch (...)
  {
    if (!default_value.empty())
    {
      return default_value;
    }
    else
    {
      cerr << "unset parameter: " << parameter_name << endl;
      abort();
    }
  }
}


string readInclude(string include_file, vector<string> search_path, string &path_out)
{
  for (auto &dir : search_path)
  {
    vector<char> content;
    string path = dir + "/" + include_file;
    if (util::readFile(path, content, true))
    {
      path_out = path;
      return string(content.data(), content.size());
    }
  }

  cout << "failed to read include file: " << include_file << endl;
  assert(0);
  abort();
}


} // namespace


namespace render_util
{


Shader::Shader(const std::string &name,
                     const std::vector<std::string> &paths_,
                     GLenum type,
                     const ShaderParameters &params) :
  m_name(name),
  m_type(type)
{
  string ext;
  string type_str;
  if (type == GL_FRAGMENT_SHADER)
  {
    ext = ".frag";
    type_str = "fragment";
  }
  else if (type == GL_VERTEX_SHADER)
  {
    ext = ".vert";
    type_str = "vertex";
  }
  else if (type == GL_GEOMETRY_SHADER)
  {
    ext = ".geom";
    type_str = "geometry";
  }
  else
    abort();

  vector<char> data;

  vector<string> paths;

  for (auto &path : paths_)
  {
    paths.push_back(path + '/' + name + ext);
    paths.push_back(path + '/' + name + ".glsl");
  }

  for (auto p : paths)
  {
    if (util::readFile(p, data, true))
    {
      cout << "sucessully read shader: " << p << endl;
      m_filename = p;
      preProcess(data, params, paths_);
      return;
    }
  }

  cerr << "Failed to read " << type_str << " shader file: " << name << endl;
}


Shader::~Shader()
{
  if (m_id)
    gl::DeleteShader(m_id);
}


void Shader::compile()
{
  if (m_preprocessed_source.empty())
    return;

  GLuint id = gl::CreateShader(m_type);
  assert(id);

  const GLchar *sources[1] = { m_preprocessed_source.c_str() };

  gl::ShaderSource(id, 1, sources, 0);
  gl::CompileShader(id);

  GLint success = 0;
  gl::GetShaderiv(id, GL_COMPILE_STATUS, &success);
  if (success !=  GL_TRUE) {
    GLint maxLength = 0;
    gl::GetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

    GLchar *infoLog = (GLchar*) malloc(maxLength);
    gl::GetShaderInfoLog(id, maxLength, &maxLength, infoLog);

    printf("Error compiling shader: %s\n%s\n", m_filename.c_str(), infoLog);
    for (int i = 0; i < m_includes.size(); i++)
      cout << "include " << i+1 << ": " << m_includes[i] << endl;

    cout << endl;

    free(infoLog);

    exit(1);
  }

  m_id = id;
}


void Shader::preProcess(const vector<char> &data_in, const ShaderParameters &params,
                        const std::vector<std::string> &paths_)
{
  string in_str(data_in.data(), data_in.size());

  istringstream in(in_str);
  string out;

  enum State
  {
    NONE,
    PARSE
  };

  State state = NONE;
  int line_num = 1;

  string parsed;
  while (in.good())
  {
    string line;
    getline(in, line);

    string trimmed = util::trim(line);
    if (trimmed.empty())
    {
      out += '\n';
      line_num++;
      continue;
    }

    if (util::isPrefix("#include", trimmed))
    {
      auto tokens = util::tokenize(trimmed);
      assert(tokens.size() == 2);
      assert(tokens.at(0) == "#include");

      auto include_file = tokens.at(1);

      string include_file_path;
      auto include_file_content = readInclude(include_file, paths_, include_file_path);
      m_includes.push_back(include_file_path);
      include_file_content = string("#line 1 ") + to_string(m_includes.size()) + "\n" + include_file_content;

      out += include_file_content;
      out += '\n';
      out += "#line " + to_string(line_num) + " 0 \n";

      line_num++;
      continue;
    }

    for (char c : line)
    {
      switch (state)
      {
        case NONE:
          assert(parsed.empty());
          if (c == '@')
          {
            state = PARSE;
          }
          else
          {
            out.push_back(c);
          }
          break;
        case PARSE:
          if (c == '@')
          {
            assert(!parsed.empty());

            out += getParameterValue(parsed, params);

            parsed.clear();

            state = NONE;
          }
          else
          {
            parsed.push_back(c);
          }
          break;
      }
    }

    assert(state == NONE);

    out += '\n';
    line_num++;
  }

  assert(state == NONE);

  m_preprocessed_source = move(out);
}


const string &ShaderParameters::get(const std::string &name) const
{
  auto it = m_map.find(name);
  if (it != m_map.end())
  {
    return it->second;
  }
  else
  {
    throw std::exception();
  }
}


void ShaderParameters::set(const std::string &name, const std::string &value)
{
  m_map[name] = value;
}


void ShaderParameters::add(const ShaderParameters &other)
{
  for (auto &entry : other.m_map)
  {
    m_map[entry.first] = entry.second;
  }
}


ShaderProgram::ShaderProgram(const std::string &name,
      const std::vector<std::string> &vertex_shaders,
      const std::vector<std::string> &fragment_shaders,
      const std::vector<std::string> &geometry_shaders,
      const std::vector<std::string> &paths,
      bool must_be_valid,
      const std::map<unsigned int, std::string> &attribute_locations,
      const ShaderParameters &parameters)
  : m_parameters(parameters),
    name(name),
    vertex_shaders(vertex_shaders),
    fragment_shaders(fragment_shaders),
    geometry_shaders(geometry_shaders),
    paths(paths),
    must_be_valid(must_be_valid),
    attribute_locations(attribute_locations)
{
  create();
  assertIsValid();
}

ShaderProgram::~ShaderProgram()
{
  cout<<"~ShaderProgram()"<<endl;
  CHECK_GL_ERROR();

  if (id)
    gl::DeleteProgram(id);

  CHECK_GL_ERROR();
}

void ShaderProgram::link()
{
  gl::LinkProgram(id);
  CHECK_GL_ERROR();

  GLint is_linked = 0;
  gl::GetProgramiv(id, GL_LINK_STATUS, (int *)&is_linked);
  if (!is_linked)
  {
    GLint maxLength = 0;
    gl::GetProgramiv(id, GL_INFO_LOG_LENGTH, &maxLength);

    GLchar *infoLog = (GLchar*) malloc(maxLength);
    gl::GetProgramInfoLog(id, maxLength, &maxLength, infoLog);

    printf("Error linking program: %s\n%s\n", name.c_str(), infoLog);

    free(infoLog);
    cerr<<"error linking"<<endl;
  }
  CHECK_GL_ERROR();

  is_valid = is_linked;
}

void ShaderProgram::create()
{
  FORCE_CHECK_GL_ERROR();

  GLint current_program_save;
  gl::GetIntegerv(GL_CURRENT_PROGRAM, &current_program_save);

  FORCE_CHECK_GL_ERROR();

  is_valid = false;

  id = gl::CreateProgram();
  assert(id != 0);

  cerr<<name<<": num fragment shaders: "<<fragment_shaders.size()<<endl;
  for (auto name : fragment_shaders)
  {
    shaders.push_back(std::move(std::make_unique<Shader>(name, paths, GL_FRAGMENT_SHADER,
                                                         m_parameters)));
  }

  cerr<<name<<": num vertex shaders: "<<vertex_shaders.size()<<endl;
  for (auto name : vertex_shaders)
  {
    shaders.push_back(std::move(std::make_unique<Shader>(name, paths, GL_VERTEX_SHADER,
                                                         m_parameters)));
  }

  cerr<<name<<": num geometry shaders: "<<geometry_shaders.size()<<endl;
  for (auto name : geometry_shaders)
  {
    shaders.push_back(std::move(std::make_unique<Shader>(name, paths, GL_GEOMETRY_SHADER,
                                                         m_parameters)));
  }

  FORCE_CHECK_GL_ERROR();

  for (auto &shader : shaders)
  {
    shader->compile();
  }

  int num_attached = 0;

  for (auto &shader : shaders)
  {
    if (!shader->getID())
      continue;

    gl::AttachShader(id, shader->getID());
    num_attached++;

    gl::Finish();
    GLenum error = gl::GetError();
    if (error != GL_NO_ERROR)
    {
      cerr<<"glAttachShader() failed for program "<<name<<", shader: "<<shader->getFileName()<<endl;
      cerr<<"gl error: "<<gl_binding::getGLErrorString(error)<<endl;
      abort();
    }
  }

  for (auto it : attribute_locations)
  {
    gl::BindAttribLocation(id, it.first, it.second.c_str());

    gl::Finish();
    GLenum error = gl::GetError();
    if (error != GL_NO_ERROR)
    {
      cerr<<"gl::BindAttribLocation() failed for program "<<name<<endl;
      cerr<<"index: "<<it.first<<", name: "<<it.second<<endl;
      cerr<<"gl error: "<<gl_binding::getGLErrorString(error)<<endl;
      abort();
    }
  }

  if (num_attached)
    link();

  gl::Finish();
  auto error = gl::GetError();
  assert(error == GL_NO_ERROR || error == GL_INVALID_VALUE);
  assert(gl::GetError() == GL_NO_ERROR);

  if (isValid())
  {
    setUniform<float>("planet_radius", planet_radius);
    setUniform("atmosphereVisibility", atmosphere_visibility);
    setUniform("atmosphereHeight", atmosphere_height);
    setUniform("max_elevation", max_elevation);
    setUniform("curvature_map_max_distance", curvature_map_max_distance);
  }

  FORCE_CHECK_GL_ERROR();
}

GLuint ShaderProgram::getId()
{
  return id;
}

void ShaderProgram::assertIsValid()
{
  assert(is_valid || !must_be_valid);
}

void ShaderProgram::assertUniformsAreSet()
{
  // slow - enable only for debugging
#if RENDER_UTIL_ENABLE_DEBUG
  int num_unset = 0;
  int num_active = 0;
  gl::GetProgramiv(id, GL_ACTIVE_UNIFORMS, &num_active);
  for (int i = 0; i < num_active; i++)
  {
    char name[1024];
    int name_length = 0;
    int size = 0;
    GLenum type = 0;
    gl::GetActiveUniform(id, i, sizeof(name), &name_length, &size, &type, name);
    string name_str(name, name_length);
    int loc = getUniformLocation(name_str);
    if (loc != -1)
    {
      if (set_uniforms.find(loc) == set_uniforms.end())
      {
        num_unset++;
        cout<<"error: " << this->name << ": unset uniform: "<<name_str<<endl;
      }
    }
  }
  if (num_unset)
    exit(1);
#endif
}

GLint ShaderProgram::getUniformLocation(const string &name)
{
  auto it = uniform_locations.find(name);
  if (it != uniform_locations.end())
    return it->second;

  GLint location = gl::GetUniformLocation(getId(), name.c_str());
  uniform_locations[name] = location;
  return location;
}

void ShaderProgram::setUniformi(GLint location, GLint value)
{
  gl::ProgramUniform1i(id, location, value);
  set_uniforms.insert(location);
}

void ShaderProgram::setUniform(int location, const bool &value)
{
  gl::ProgramUniform1i(id, location, value);
  set_uniforms.insert(location);
}

void ShaderProgram::setUniform(GLint location, const GLfloat &value)
{
  gl::ProgramUniform1f(id, location, value);
  set_uniforms.insert(location);
}

void ShaderProgram::setUniform(GLint location, const glm::vec2 &value)
{
  gl::ProgramUniform2fv(id, location, 1, value_ptr(value));
  set_uniforms.insert(location);
}

void ShaderProgram::setUniform(GLint location, const glm::vec3 &value)
{
  gl::ProgramUniform3fv(id, location, 1, value_ptr(value));
  set_uniforms.insert(location);
}

void ShaderProgram::setUniform(GLint location, const glm::vec4 &value)
{
  gl::ProgramUniform4fv(id, location, 1, value_ptr(value));
  set_uniforms.insert(location);
}

void ShaderProgram::setUniform(GLint location, const glm::ivec2 &value)
{
  gl::ProgramUniform2iv(id, location, 1, value_ptr(value));
  set_uniforms.insert(location);
}

void ShaderProgram::setUniform(int location, const glm::mat3 &value)
{
  gl::ProgramUniformMatrix3fv(id, location, 1, false, value_ptr(value));
  set_uniforms.insert(location);
}

void ShaderProgram::setUniform(GLint location, const glm::mat4 &value)
{
  gl::ProgramUniformMatrix4fv(id, location, 1, false, value_ptr(value));
  set_uniforms.insert(location);
}


} // namespace render_util
