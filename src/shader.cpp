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
#include <gl_wrapper/gl_functions.h>

using namespace gl_wrapper::gl_functions;
using namespace std;
using namespace glm;
using namespace render_util;

namespace
{


string preProcessShader(const vector<char> &in, const ShaderParameters &params)
{
  string out;

  enum State
  {
    NONE,
    PARSE
  };

  State state = NONE;

  string parsed;

  for (char c : in)
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

          out += std::to_string(params.get(parsed));

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

  return move(out);
}


string readShaderFile(string file_path, const ShaderParameters &params)
{

  string content;

  vector<char> data;
  if (util::readFile(file_path, data))
  {
    content = preProcessShader(data, params);
  }
  else
  {
    printf("Failed to read %s\n", file_path.c_str());
  }

  return content;
}


GLuint createShader(const string &name,
                    const string &path,
                    GLenum type,
                    const ShaderParameters &params)
{
  string filename = name;
  if (type == GL_FRAGMENT_SHADER)
    filename += ".frag";
  else if (type == GL_VERTEX_SHADER)
    filename += ".vert";
  else
    abort();

//     string header = readShaderFile("common.glsl");
//     assert(!header.empty());

  string source = readShaderFile(path + '/' + filename, params);
  if (source.empty()) {
    return 0;
  }

  GLuint id = gl::CreateShader(type);
  assert(id);
  
//     const int num_sources = 2;

  const GLchar *sources[1] = { source.c_str() };
//     const GLchar *sources[num_sources] = { header.c_str(),  source.c_str() };

  gl::ShaderSource(id, 1, sources, 0);
//     glShaderSource(id, num_sources, sources, 0);
  gl::CompileShader(id);

  GLint success = 0;
  gl::GetShaderiv(id, GL_COMPILE_STATUS, &success);
  if (success !=  GL_TRUE) {
    GLint maxLength = 0;
    gl::GetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

    GLchar *infoLog = (GLchar*) malloc(maxLength);
    gl::GetShaderInfoLog(id, maxLength, &maxLength, infoLog);

    printf("Error compiling shader: %s\n%s\n", filename.c_str(), infoLog);

    free(infoLog);

    exit(1);
  }

  return id;
}


} // namespace


namespace render_util
{

int ShaderParameters::get(const std::string &name) const
{
  auto it = m_map.find(name);
  if (it != m_map.end())
  {
    return it->second;
  }
  else
  {
    cerr << "unset parameter: " << name << endl;
    abort();
  }
}


void ShaderParameters::set(const std::string &name, int value)
{
  m_map[name] = value;
}


ShaderProgram::ShaderProgram( const string &name,
                              const std::vector<std::string> &vertex_shaders,
                              const std::vector<std::string> &fragment_shaders,
                              const std::string &path,
                              bool must_be_valid,
                              const std::map<unsigned int, std::string> &attribute_locations,
                              const ShaderParameters &parameters)
  : m_parameters(parameters),
    name(name),
    vertex_shaders(vertex_shaders),
    fragment_shaders(fragment_shaders),
    path(path),
    must_be_valid(must_be_valid),
    attribute_locations(attribute_locations)
{
  create();
  assertIsValid();
}

ShaderProgram::ShaderProgram(const char *name, const string &path, bool must_be_valid) :
  id(0), path(path), must_be_valid(must_be_valid)
{
  this->name = name;
  fragment_shaders.push_back("main");
  fragment_shaders.push_back(name);
  vertex_shaders.push_back(name);
  create();
  assertIsValid();
}

ShaderProgram::ShaderProgram(const char *name_vert, const char *name_frag, const string &path, bool must_be_valid) :
  id(0), path(path), must_be_valid(must_be_valid)
{
  name = name_frag;
  fragment_shaders.push_back("main");
  fragment_shaders.push_back(name_frag);
  vertex_shaders.push_back(name_vert);
  create();
  assertIsValid();
}

ShaderProgram::~ShaderProgram()
{
  cout<<"~ShaderProgram()"<<endl;
  CHECK_GL_ERROR();

  assert(id);

  for (auto shader : shader_objects)
  {
    gl::DetachShader(id, shader);
    gl::DeleteShader(shader);
    CHECK_GL_ERROR();
  }

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
  gl::Finish();
  GLenum error = gl::GetError();
  if (error != GL_NO_ERROR)
  {
    cerr<<"gl error: "<<gl_wrapper::getGLErrorString(error)<<endl;
    abort();
  }
  
  GLint current_program_save;
  gl::GetIntegerv(GL_CURRENT_PROGRAM, &current_program_save);

  gl::Finish();
  error = gl::GetError();
  if (error != GL_NO_ERROR)
  {
    cerr<<"gl error: "<<gl_wrapper::getGLErrorString(error)<<endl;
    abort();
  }

  is_valid = false;

  id = gl::CreateProgram();
  assert(id != 0);

  cerr<<name<<"num fragment shaders: "<<fragment_shaders.size()<<endl;
  for (auto name : fragment_shaders)
  {
    GLuint shader = createShader(name, path, GL_FRAGMENT_SHADER, m_parameters);
    if (!shader)
    {
      cerr<<"failed to create shader: "<<name<<endl;
      return;
    }
    else
    {
      shader_objects.push_back(shader);
    }
  }

  cerr<<name<<": num vertex shaders: "<<vertex_shaders.size()<<endl;
  for (auto name : vertex_shaders)
  {
    GLuint shader = createShader(name, path, GL_VERTEX_SHADER, m_parameters);
    if (!shader)
    {
      cerr<<"failed to create shader: "<<name<<endl;
      return;
    }
    else
    {
      shader_objects.push_back(shader);
    }
  }

  gl::Finish();
  error = gl::GetError();
  if (error != GL_NO_ERROR)
  {
    cerr<<"gl error: "<<gl_wrapper::getGLErrorString(error)<<endl;
    abort();
  }

  for (auto shader : shader_objects)
  {
    gl::AttachShader(id, shader);

    gl::Finish();
    GLenum error = gl::GetError();
    if (error != GL_NO_ERROR)
    {
      cerr<<"glAttachShader() failed for program "<<name<<endl;
      cerr<<"gl error: "<<gl_wrapper::getGLErrorString(error)<<endl;
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
      cerr<<"gl error: "<<gl_wrapper::getGLErrorString(error)<<endl;
      abort();
    }
  }

  link();

  gl::Finish();
  error = gl::GetError();
  assert(error == GL_NO_ERROR || error == GL_INVALID_VALUE);
  assert(gl::GetError() == GL_NO_ERROR);

  if (isValid())
  {
    setUniform("planet_radius", planet_radius);
    setUniform("atmosphereVisibility", atmosphere_visibility);
    setUniform("atmosphereHeight", atmosphere_height);
    setUniform("max_elevation", max_elevation);
    setUniform("curvature_map_max_distance", curvature_map_max_distance);
  }

  CHECK_GL_ERROR();
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
#if 0
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

void ShaderProgram::setUniform(GLint location, const glm::mat4 &value)
{
  gl::ProgramUniformMatrix4fv(id, location, 1, GL_FALSE, value_ptr(value));
  set_uniforms.insert(location);
}

} // namespace render_util
