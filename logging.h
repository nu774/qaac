#include <cstdio>
#include <cstdarg>
#include <vector>
#include "platformutil.h"

class Log {
    std::vector<std::shared_ptr<FILE>> m_streams;
public:
    static Log &instance()
    {
        static Log self;
        return self;
    }
    bool is_enabled() { return m_streams.size() != 0; }
    void enable_stderr()
    {
#ifdef _WIN32
        // A GUI-subsystem process (or one otherwise launched with no
        // console) can have an invalid stderr handle; skip logging to it
        // in that case rather than fail later on every write.
        if (GetFileType(platform::get_handle(2)) != FILE_TYPE_UNKNOWN)
            m_streams.push_back(std::shared_ptr<FILE>(stderr, [](FILE*){}));
#else
        m_streams.push_back(std::shared_ptr<FILE>(stderr, [](FILE*){}));
#endif
    }
    void enable_file(const std::string &filename)
    {
        try {
            FILE *fp = platform::wfopenx(filename, "w");
            std::setbuf(fp, 0);
            m_streams.push_back(std::shared_ptr<FILE>(fp, std::fclose));
        } catch (...) {}
    }
    void vprintf(const char *fmt, va_list args)
    {
        va_list args2;
        va_copy(args2, args);
#ifdef _WIN32
        int rc = _vscprintf(fmt, args);
#else
        int rc = std::vsnprintf(nullptr, 0, fmt, args);
#endif
        std::vector<char> buffer(rc + 1);
        vsnprintf(buffer.data(), buffer.size(), fmt, args2);
        va_end(args2);

#ifdef _WIN32
        OutputDebugStringA(buffer.data());
#endif
        for (size_t i = 0; i < m_streams.size(); ++i)
            platform::write_utf8(m_streams[i].get(), buffer.data());
    }
    void printf(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }
private:
    Log() {}
    Log(const Log&);
    Log& operator=(const Log&);
};

#ifdef __GNUC__
#define LOG(fmt, ...) Log::instance().printf(fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...) Log::instance().printf(fmt, __VA_ARGS__)
#endif