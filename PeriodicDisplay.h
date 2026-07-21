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
    std::wstring m_message;
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
    void put(const std::wstring &message) {
        m_message = message;
        uint32_t tick = GetTickCount();
        if (tick - m_last_tick_stderr > m_interval) {
            m_last_tick_stderr = tick;
            flush();
        }
    }
    void flush() {
        if (m_verbose) std::fputws(m_message.c_str(), stderr);
        if (m_verbose && m_console_visible &&
            m_last_tick_stderr - m_last_tick_title > m_interval * 4)
        {
            std::vector<wchar_t> s(m_message.size() + 1);
            std::wcscpy(&s[0], m_message.c_str());
            strutil::squeeze(&s[0], L"\r");
            std::wstring msg = strutil::format(L"%hs %s", PROGNAME, &s[0]);
            SetConsoleTitleW(msg.c_str());
            m_last_tick_title = m_last_tick_stderr;
        }
    }
};

#endif
