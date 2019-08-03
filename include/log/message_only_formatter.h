/*
  Based on plog/Formatters/MessageOnlyFormatter.h
  Copyright (c) 2016 Sergey Podobry (sergey.podobry at gmail.com).
  Documentation and sources: https://github.com/SergiusTheBest/plog
  License: MPL 2.0, http://mozilla.org/MPL/2.0/
*/

#ifndef UTIL_LOG_MESSAGE_ONLY_FORMATTER_H
#define UTIL_LOG_MESSAGE_ONLY_FORMATTER_H

#include <plog/Record.h>
#include <plog/Util.h>

namespace util::log
{
    template <bool ADD_NEW_LINE>
    class MessageOnlyFormatter
    {
    public:
        static plog::util::nstring header()
        {
            return plog::util::nstring();
        }

        static plog::util::nstring format(const plog::Record& record)
        {
            plog::util::nostringstream ss;
            ss << record.getMessage();

            if (ADD_NEW_LINE)
            {
              ss << PLOG_NSTR("\n");
            }

            return ss.str();
        }
    };
}

#endif
