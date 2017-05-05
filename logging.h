#include <cstdio>
#include <cstdarg>
#include <vector>
#include "win32util.h"

/* non-thread safe */
class Log {
    std::vector<std::shared_ptr<FILE> > m_streams;
    DWORD m_stderr_type;
    static Log *m_instance;
public:
    static Log *instance()
    {
        if (!m_instance) m_instance = new Log();
        return m_instance;
    }
    bool is_enabled() { return m_streams.size() != 0; }
    void enable_stderr()
    {
        if (m_stderr_type != FILE_TYPE_UNKNOWN)
            m_streams.push_back(std::shared_ptr<FILE>(stderr, [](FILE*){}));
    }
    void enable_file(const wchar_t *filename)
    {
        try {
            FILE *fp = win32::wfopenx(filename, L"w");
            _setmode(_fileno(fp), _O_U8TEXT);
            std::setbuf(fp, 0);
            m_streams.push_back(std::shared_ptr<FILE>(fp, std::fclose));
        } catch (...) {}
    }
    void vwprintf(const wchar_t *fmt, va_list args)
    {
        int rc = _vscwprintf(fmt, args);
        std::vector<wchar_t> buffer(rc + 1);
        rc = _vsnwprintf(buffer.data(), buffer.size(), fmt, args);

        OutputDebugStringW(buffer.data());
        for (size_t i = 0; i < m_streams.size(); ++i)
            std::fputws(buffer.data(), m_streams[i].get());
    }
private:
    Log()
    {
        m_stderr_type = GetFileType(win32::get_handle(_fileno(stderr)));
    }
};

inline void LOG(const wchar_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Log::instance()->vwprintf(fmt, ap);
    va_end(ap);
}

inline bool IS_LOG_ENABLED()
{
    return Log::instance()->is_enabled();
}
