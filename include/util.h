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
#include <fstream>
#include <cstdio>
#include <cmath>
#include <cassert>

namespace util
{

constexpr const long double PI = acosl((long double)-1.0);

inline bool isPrefix(const std::string &prefix, const std::string &s)
{
  return s.compare(0, prefix.size(), prefix) == 0;
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


inline bool readFile(const std::string &path, std::vector<char> &content)
{
  bool success = false;

  std::ifstream file(path);
  if (file.good())
  {
    file.seekg (0, file.end);
    size_t size = file.tellg();
    file.seekg (0, file.beg);
    content.resize(size);
    file.read(content.data(), size);
    assert(file.good());
    success = true;
  } 
  else
  {
      printf("Failed to open %s\n", path.c_str());
  }

  return success;
}


} // namespace util

#endif
