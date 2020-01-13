#include <file.h>
#include <log.h>

namespace util
{

NormalFile::NormalFile(std::string path) : m_path(path)
{
  m_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
  m_stream.open(m_path, std::ios_base::binary);
  assert(m_stream.good());

  m_stream.seekg (0, m_stream.end);
  m_size = m_stream.tellg();
  m_stream.seekg (0, m_stream.beg);
}


void NormalFile::checkState()
{
}


int NormalFile::read(char *out, int bytes)
{
  assert(m_stream.good());
  try
  {
    m_stream.read(out, bytes);
    return m_stream.gcount();
  }
  catch (std::ios_base::failure &e)
  {
//     LOG_ERROR << e.what() << std::endl;
    if (!m_stream.eof() || m_stream.bad())
    {
//       LOG_ERROR << "giving up." << std::endl;
      throw;
    }
    else
    {
//       LOG_ERROR << "continuing." << std::endl;
      assert(m_stream.eof());
      return m_stream.gcount();
    }
  }
}

void NormalFile::skip(int bytes)
{
  m_stream.seekg(bytes, std::ios_base::cur);
}

void NormalFile::rewind()
{
  m_stream.clear();
  m_stream.seekg(0, std::ios_base::beg);
}

bool NormalFile::eof()
{
  return m_stream.eof();
}


void NormalFile::readAll(std::vector<char> &out)
{
  bool success = false;

  if (m_stream.good())
  {
    m_stream.seekg (0, m_stream.end);
    size_t size = m_stream.tellg();
    m_stream.seekg (0, m_stream.beg);
    out.resize(size);
    m_stream.read(reinterpret_cast<char*>(out.data()), out.size());

    if (m_stream.good())
      success = true;
  }

  if (!success)
  {
    throw std::runtime_error("Failed to read " + m_path);
  }
}


int NormalFile::getSize()
{
  return m_size;
}


}
