/**
 *    Rendering utilities
 *    Copyright (C) 2020 Jan Lepper
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

#ifndef UTIL_FILE_BASE_H
#define UTIL_FILE_BASE_H

#include <vector>
#include <string>
#include <fstream>

namespace util
{


struct File
{
  virtual ~File() {}
  virtual int read(char *out, int bytes) = 0;
  virtual void skip(int bytes) = 0;
  virtual void rewind() = 0;
  virtual bool eof() = 0;
  virtual void readAll(std::vector<char>&) = 0;
  virtual int getSize() = 0;
};


class NormalFile : public File
{
  std::ifstream m_stream;
  std::string m_path;
  int m_size = 0;

  void checkState();

public:
  NormalFile(std::string path);

  int read(char *out, int bytes) override;
  void skip(int bytes) override;
  void rewind() override;
  bool eof() override;
  void readAll(std::vector<char>&) override;
  int getSize() override;
};


}

#endif
