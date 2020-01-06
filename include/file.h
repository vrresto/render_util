#ifndef UTIL_FILE_BASE_H
#define UTIL_FILE_BASE_H

#include <string>
#include <fstream>

namespace util
{


struct File
{
  virtual int read(char *out, int bytes) = 0;
  virtual void skip(int bytes) = 0;
  virtual void rewind() = 0;
  virtual bool eof() = 0;
};


class NormalFile : public File
{
  std::ifstream m_stream;
  std::string m_path;

  void checkState();

public:
  NormalFile(std::string path);

  int read(char *out, int bytes) override;
  void skip(int bytes) override;
  void rewind() override;
  bool eof() override;
};


}

#endif
