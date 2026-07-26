#ifndef PERIODICDISPLAY_H
#define PERIODICDISPLAY_H

#include <cstdint>
#include <string>
#include "win32util.h"
#include "strutil.h"

#ifndef PROGNAME
#define PROGNAME "qaac"
#endif

class PeriodicDisplay {
    uint32_t m_interval;
    uint32_t m_last_tick_title;
    uint32_t m_last_tick_stderr;
    std::string m_message;
    bool m_verbose;
    bool m_console_visible;
public:
    PeriodicDisplay(uint32_t interval, bool verbose=true)
        : m_interval(interval),
          m_verbose(verbose)
    {
        m_console_visible = IsWindowVisible(GetConsoleWindow());
        m_last_tick_title = m_last_tick_stderr = GetTickCount();
    }
    void put(const std::string &message) {
        m_message = message;
        uint32_t tick = GetTickCount();
        if (tick - m_last_tick_stderr > m_interval) {
            m_last_tick_stderr = tick;
            flush();
        }
    }
    void flush() {
        if (m_verbose) win32::write_utf8(stderr, m_message);
        if (m_verbose && m_console_visible &&
            m_last_tick_stderr - m_last_tick_title > m_interval * 4)
        {
            std::string s = m_message;
            strutil::squeeze(&s[0], "\r");
            std::string msg = strutil::format("%s %s", PROGNAME, s.c_str());
            SetConsoleTitleW(strutil::us2w(msg).c_str());
            m_last_tick_title = m_last_tick_stderr;
        }
    }
};

#endif
