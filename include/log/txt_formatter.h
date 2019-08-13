/*
  Based on plog/Formatters/TxtFormatter.h
  Copyright (c) 2016 Sergey Podobry (sergey.podobry at gmail.com).
  Documentation and sources: https://github.com/SergiusTheBest/plog
  License: MPL 2.0, http://mozilla.org/MPL/2.0/
*/

#ifndef UTIL_LOG_TXT_FORMATTER_H
#define UTIL_LOG_TXT_FORMATTER_H

#include <plog/Record.h>
#include <plog/Util.h>
#include <iomanip>

namespace util::log
{
    template<bool useUtcTime, bool addNewLine>
    class TxtFormatterImpl
    {
    public:
        static plog::util::nstring header()
        {
            return plog::util::nstring();
        }

        static plog::util::nstring format(const plog::Record& record)
        {
            tm t;
            (useUtcTime ? plog::util::gmtime_s : plog::util::localtime_s)(&t, &record.getTime().time);

            plog::util::nostringstream ss;

            if (!record.isTextOnly())
            {
              ss << t.tm_year + 1900 << "-"
                << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mon + 1 << PLOG_NSTR("-")
                << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_mday << PLOG_NSTR(" ");

              ss << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_hour << PLOG_NSTR(":")
                << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_min << PLOG_NSTR(":")
                << std::setfill(PLOG_NSTR('0')) << std::setw(2) << t.tm_sec << PLOG_NSTR(".")
                << std::setfill(PLOG_NSTR('0')) << std::setw(3) << record.getTime().millitm
                << PLOG_NSTR(" ");

              ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left
                << severityToString(record.getSeverity()) << PLOG_NSTR(" ");

              ss << PLOG_NSTR("[") << record.getTid() << PLOG_NSTR("] ");

              ss << PLOG_NSTR("[") << record.getFunc() << PLOG_NSTR(":")
                << record.getFile() << PLOG_NSTR("@") << record.getLine() << PLOG_NSTR("] ");
            }

            ss << record.getMessage();

            if (addNewLine)
            {
              ss << PLOG_NSTR("\n");
            }

            return ss.str();
        }
    };

    template<bool addNewLine>
    class TxtFormatter : public TxtFormatterImpl<false, addNewLine> {};

    template<bool addNewLine>
    class TxtFormatterUtcTime : public TxtFormatterImpl<true, addNewLine> {};
}

#endif
