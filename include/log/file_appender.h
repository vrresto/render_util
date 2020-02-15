/**
 *    Rendering utilities
 *    Copyright (C) 2019 Jan Lepper
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

#ifndef UTIL_LOG_FILE_APPENDER_H

#include <plog/Appenders/IAppender.h>
#include <plog/Converters/UTF8Converter.h>
#include <plog/Util.h>
#include <fstream>

namespace util::log
{
  template<class Formatter>
  class FileAppender : public plog::IAppender
  {
    using Converter = plog::UTF8Converter;

  public:
    FileAppender(std::string file_name) : m_out(file_name)
    {
      if (!m_out.good())
      {
        throw std::runtime_error("Failed to open log file for writing: " + file_name);
      }
    }

    virtual void write(const plog::Record& record)
    {
      auto str = Converter::convert(Formatter::format(record));
      plog::util::MutexLock lock(m_mutex);

      m_out << str << std::flush;
    }

  private:
    plog::util::Mutex m_mutex;
    std::ofstream m_out;
  };
}

#endif
