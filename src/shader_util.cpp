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

#include <render_util/shader_util.h>
#include <render_util/texunits.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <render_util/gl_binding/gl_functions.h>

using namespace std;

namespace render_util
{


ShaderProgramPtr createShaderProgram(const std::string &definition,
                                     const render_util::TextureManager &tex_mgr,
                                     const std::string &shader_path,
                                     const std::map<unsigned int, std::string> &attribute_locations,
                                     const ShaderParameters &params)
{
  cout<<"creating shader program: "<<definition<<endl;

  vector<string> vertex_shaders;
  vector<string> fragment_shaders;
  vector<string> texunits;

  {
    string path = shader_path + '/' + definition + ".program";
    ifstream in(path);
    if (!in.good())
    {
      cout<<"failed to open "<<path<<endl;
    }
    assert(in.good());

    while (in.good())
    {
      string line;
      getline(in, line);
      if (line.empty())
        continue;
      if (line[0] == '#')
        continue;

      istringstream line_in(line);

      assert(line_in.good());
      string type;
      line_in >> type;
      assert(!type.empty());

      assert(line_in.good());
      string name;
      line_in >> name;
      assert(!name.empty());

      if (type == "vert")
        vertex_shaders.push_back(name);
      else if (type == "frag")
        fragment_shaders.push_back(name);
      else if (type == "texunit")
        texunits.push_back(name);
      else
      {
        assert(false);
      }
    }
  }

  assert(!fragment_shaders.empty());

  ShaderProgramPtr program = make_shared<ShaderProgram>(definition, vertex_shaders, fragment_shaders, shader_path, true, attribute_locations, params);

  for (auto name : texunits)
  {
    int number = getTexUnitNumber(name);
    assert(number >= 0);

    program->setUniformi("sampler_" + name, tex_mgr.getTexUnitNum(number));
  }

  CHECK_GL_ERROR(); 

  return program;
}

ShaderProgramPtr createSkyProgram(const render_util::TextureManager &tex_mgr, const string &shader_path)
{
  return createShaderProgram("sky", tex_mgr, shader_path);
}


} // namespace
