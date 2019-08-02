/*
  Based on plog/Appenders/ColorConsoleAppender.h
  Copyright (c) 2016 Sergey Podobry (sergey.podobry at gmail.com).
  Documentation and sources: https://github.com/SergiusTheBest/plog
  License: MPL 2.0, http://mozilla.org/MPL/2.0/
*/

#ifndef UTIL_LOG_COLOR_CONSOLE_APPENDER_H
#define UTIL_LOG_COLOR_CONSOLE_APPENDER_H

#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/MessageOnlyFormatter.h>

namespace util::log
{
    template<class Formatter>
    class ColorConsoleAppenderUnix : public plog::ConsoleAppender<Formatter>
    {
    public:
        ColorConsoleAppenderUnix() {}

        virtual void write(const plog::Record& record)
        {
            plog::util::nstring str = Formatter::format(record);
            plog::util::MutexLock lock(this->m_mutex);

            setColor(record.getSeverity());
            this->writestr(str);
            resetColor();
        }

    private:
        void setColor(plog::Severity severity)
        {
            if (this->m_isatty)
            {
                switch (severity)
                {
                case plog::fatal:
                    std::cout << "\x1B[97m\x1B[41m"; // white on red background
                    break;

                case plog::error:
                    std::cout << "\x1B[91m"; // red
                    break;

                case plog::warning:
                    std::cout << "\x1B[93m"; // yellow
                    break;

                case plog::debug:
                case plog::verbose:
                    std::cout << "\x1B[96m"; // cyan
                    break;
                default:
                    break;
                }
            }
        }

        void resetColor()
        {
            if (this->m_isatty)
            {
                std::cout << "\x1B[0m\x1B[0K";
            }
        }
    };
}

#endif
