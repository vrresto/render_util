#ifndef UTIL_LOG_CONSOLE_APPENDER_H
#define UTIL_LOG_CONSOLE_APPENDER_H

#include <render_util/config.h>
#include <log/color_console_appender_unix.h>

#include <plog/Appenders/ConsoleAppender.h>

namespace util::log
{
  template <class T>
#if USE_UNIX_CONSOLE
  using ConsoleAppender = ColorConsoleAppenderUnix<T>;
#else
  using ConsoleAppender = plog::ConsoleAppender<T>;
#endif
}

#endif
