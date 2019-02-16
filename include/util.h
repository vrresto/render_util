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

#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cmath>
#include <cassert>

namespace util
{

constexpr long double PI = acosl((long double)-1.0);


inline bool isPrefix(const std::string &prefix, const std::string &s)
{
  return s.compare(0, prefix.size(), prefix) == 0;
}


inline std::string trim(const std::string &in)
{
  size_t start = in.find_first_not_of(" \t\n\r");
  if (start == std::string::npos)
    return {};

  size_t end = in.find_last_not_of(" \t\n\r");
  assert(end != std::string::npos);

  if (start >= end+1)
  {
    std::cout<<"start: "<<start<<", end: "<<end<<std::endl;
  }

  assert(start < end+1);

  size_t len = end+1 - start;
  assert(len);

  return std::move(in.substr(start, len));
}


inline std::vector<std::string> tokenize(std::string in, const std::string &separators = " \t\n\r")
{
  std::vector<std::string> out;

  for (size_t token_start = 0, token_len = 0; token_start < in.size(); )
  {
    size_t token_end = token_start + token_len;

    if (token_end >= in.size())
    {
      if (token_len)
      {
        std::string token = in.substr(token_start);
        out.push_back(std::move(token));
      }
      break;
    }
    else if (separators.find(in[token_end]) != std::string::npos)
    {
      if (token_len)
      {
        std::string token = in.substr(token_start, token_len);
        out.push_back(std::move(token));
      }

      token_start = token_end + 1;
      token_len = 0;
    }
    else
    {
      token_len++;
    }
  }

  return std::move(out);
}


inline std::vector<std::string> tokenize(std::string in, char separator)
{
  return std::move(tokenize(in, std::string {separator}));
}


inline std::string makeLowercase(const std::string &input)
{
  std::string output = input;
  for(size_t i = 0; i < output.size(); i++)
  {
    output[i] = tolower(output[i]);
  }
  return output;
}


inline std::string getDirFromPath(const std::string &path)
{
  std::string dir;
  size_t pos = path.find_last_of("/\\");
  if (pos != std::string::npos)
  {
    if (pos < path.size() && pos > 0)
      dir = path.substr(0, pos);
  }

  return dir;
}


inline std::string basename(std::string path, bool remove_extension = false)
{
  size_t pos = path.find_last_of("/\\");
  if (pos != std::string::npos)
  {
    if (pos < path.size())
      path = path.substr(pos + 1);
    else
      path = {};
  }

  if (remove_extension)
    path = path.substr(0, path.find_last_of('.'));

  return path;
}


inline std::string resolveRelativePathComponents(const std::string &path)
{
  auto components = tokenize(path, "/\\");
  assert(!components.empty());

  std::string resolved;

  std::vector<std::string> new_components;

  for (auto &component : components)
  {
    if (component == "..")
    {
      assert(!new_components.empty());
      new_components.pop_back();
    }
    else
    {
      new_components.push_back(component);
    }
  }

  assert(!new_components.empty());

  for (auto &component : new_components)
  {
    if (!resolved.empty())
      resolved += '/';
    resolved += component;
  }

  return std::move(resolved);
}


inline bool readFile(const std::string &path, std::vector<char> &content, bool quiet = false)
{
  using namespace std;

  bool success = false;

  std::ifstream file(path, ios_base::binary);
  if (file.good())
  {
    file.seekg (0, file.end);
    size_t size = file.tellg();
    file.seekg (0, file.beg);
    content.resize(size);
    file.read(content.data(), size);

    if (file.good())
      success = true;
    else if (!quiet)
      fprintf(stderr, "Failed to read %s\n", path.c_str());
  }
  else if (!quiet)
  {
      fprintf(stderr, "Failed to open %s\n", path.c_str());
  }

  return success;
}


inline bool writeFile(const std::string &path, const char *data, size_t data_size)
{
  using namespace std;

  std::ofstream out(path, ios_base::binary | ios_base::trunc);
  if (!out.good()) {
    cerr<<"can't open output file "<<path<<endl;
    return false;
  }

  assert(out.tellp() == 0);

  out.write(data, data_size);

  size_t size = out.tellp();
  cout<<"data_size:"<<data_size<<endl;
  cout<<"size:"<<size<<endl;
  assert(data_size == size);

  if (!out.good()) {
    cerr<<"error during writing to output file "<<path<<endl;
    return false;
  }

  return true;
}


bool mkdir(const char *name);


} // namespace util

#endif
