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

#include <log.h>

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


const auto PI = std::acos(-1.l);


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
    LOG_INFO<<"start: "<<start<<", end: "<<end<<std::endl;
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


inline std::string getFileExtensionFromPath(const std::string &path)
{
  auto pos = path.find_last_of('.');
  if (pos != std::string::npos)
    return path.substr(pos + 1, std::string::npos);
  else
    return {};
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


template <typename T>
inline std::vector<T> readFile(const std::string &path)
{
  static_assert(sizeof(T) == 1);
  using namespace std;

  bool success = false;

  std::vector<T> content;

  std::ifstream file(path, ios_base::binary);
  if (file.good())
  {
    file.seekg (0, file.end);
    size_t size = file.tellg();
    file.seekg (0, file.beg);
    content.resize(size);
    file.read(reinterpret_cast<char*>(content.data()), size);

    if (file.good())
      success = true;
  }

  if (!success)
  {
    throw std::runtime_error("Failed to read " + path);
  }

  return std::move(content);
}


template <typename T>
inline bool readFile(const std::string &path, T& content, bool quiet = false)
{
  using ElementType = typename std::remove_const<typename T::value_type>::type;
  static_assert(sizeof(ElementType) == 1);

  using namespace std;

  bool success = false;

  std::ifstream file(path, ios_base::binary);
  if (file.good())
  {
    file.seekg (0, file.end);
    size_t size = file.tellg();
    file.seekg (0, file.beg);
    content.resize(size);
    file.read(reinterpret_cast<char*>(content.data()), size);

    if (file.good())
      success = true;
  }

  if (!success)
  {
    if (!quiet)
      LOG_ERROR << "Failed to read " << path << endl;
    else
      LOG_TRACE << "Failed to read " << path << endl;
  }

  return success;
}


inline bool writeFile(const std::string &path, const char *data, int data_size)
{
  using namespace std;

  std::ofstream out(path, ios_base::binary | ios_base::trunc);
  if (!out.good()) {
    LOG_ERROR<<"can't open output file "<<path<<endl;
    return false;
  }

  assert(out.tellp() == 0);

  out.write(data, data_size);

  size_t size = out.tellp();
  LOG_INFO<<"data_size:"<<data_size<<endl;
  LOG_INFO<<"size:"<<size<<endl;
  assert(data_size == size);

  if (!out.good()) {
    LOG_ERROR<<"error during writing to output file "<<path<<endl;
    return false;
  }

  return true;
}


bool mkdir(const char *name);
bool fileExists(std::string path);

std::string makeTimeStampString();


} // namespace util

#endif
