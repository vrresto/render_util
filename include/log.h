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

#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <render_util/config.h>

#if USE_PLOG

#include <plog/Log.h>
#include <glm/glm.hpp>

namespace util::log
{
  enum
  {
    LOGGER_DEFAULT = PLOG_DEFAULT_INSTANCE,
    LOGGER_WARNING,
    LOGGER_INFO,
    LOGGER_DEBUG,
    LOGGER_TRACE
  };

  inline void separator()
  {
    plog::Record record(plog::error);
    record << "-----------------------------------------------------------" << std::endl;
    *plog::get<PLOG_DEFAULT_INSTANCE>() += record;
  }

  template <typename T>
  plog::Record& putVec2(plog::Record &record, const T &arg)
  {
    record << '[' << arg.x << ", " << arg.y << ']';
    return record;
  }

  template <typename T>
  plog::Record& putVec3(plog::Record &record, const T &arg)
  {
    record << '[' << arg.x << ", " << arg.y << ", " << arg.z << ']';
    return record;
  }

  template <typename T>
  plog::Record& putVec4(plog::Record &record, const T &arg)
  {
    record << '[' << arg.x << ", " << arg.y << ", " << arg.z << ", " << arg.w << ']';
    return record;
  }
}


inline plog::Record& operator<<(plog::Record &record, const glm::ivec2 &arg)
{
  return util::log::putVec2(record, arg);
}

inline plog::Record& operator<<(plog::Record &record, const glm::ivec3 &arg)
{
  return util::log::putVec3(record, arg);
}

inline plog::Record& operator<<(plog::Record &record, const glm::ivec4 &arg)
{
  return util::log::putVec4(record, arg);
}

inline plog::Record& operator<<(plog::Record &record, const glm::vec2 &arg)
{
  return util::log::putVec2(record, arg);
}

inline plog::Record& operator<<(plog::Record &record, const glm::vec3 &arg)
{
  return util::log::putVec3(record, arg);
}

inline plog::Record& operator<<(plog::Record &record, const glm::vec4 &arg)
{
  return util::log::putVec4(record, arg);
}

inline plog::Record& operator<<(plog::Record &record, const glm::dvec2 &arg)
{
  return util::log::putVec2(record, arg);
}

inline plog::Record& operator<<(plog::Record &record, const glm::dvec3 &arg)
{
  return util::log::putVec3(record, arg);
}

inline plog::Record& operator<<(plog::Record &record, const glm::dvec4 &arg)
{
  return util::log::putVec4(record, arg);
}


#define LOG_TRACE PLOG_VERBOSE
#define LOG_DEBUG PLOG_DEBUG
#define LOG_INFO PLOG_INFO
#define LOG_WARNING PLOG_WARNING
#define LOG_ERROR PLOG_ERROR

#define LOG_SEPARATOR util::log::separator()
#define LOG_FLUSH {}

#else // USE_PLOG

#include <iostream>

#define LOG_TRACE std::cout
#define LOG_DEBUG std::cout
#define LOG_INFO std::cout
#define LOG_WARNING std::cout
#define LOG_ERROR std::cout

#define LOG_SEPARATOR {}
#define LOG_FLUSH std::cout << std::flush

#endif // USE_PLOG

#endif
