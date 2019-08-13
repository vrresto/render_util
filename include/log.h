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

namespace util::log
{
  enum
  {
    LOGGER_DEFAULT = PLOG_DEFAULT_INSTANCE,
    LOG_SINK_WARNING,
    LOG_SINK_INFO,
    LOG_SINK_DEBUG,
    LOG_SINK_TRACE
  };
}

#define LOG_TRACE PLOG_VERBOSE
#define LOG_DEBUG PLOG_DEBUG
#define LOG_INFO PLOG_INFO
#define LOG_WARNING PLOG_WARNING
#define LOG_ERROR PLOG_ERROR

#define LOG_SEPARATOR {}
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
