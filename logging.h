#include <cstdio>
#include <cstdarg>
#include <vector>
#include "win32util.h"
#include "shared_ptr.h"

/* non-thread safe */
class Log {
    std::vector<x::shared_ptr<FILE> > m_streams;
    static Log *m_instance;
    Log() {}
public:
    static Log *instance()
    {
	if (!m_instance) m_instance = new Log();
	return m_instance;
    }
    void enable_stderr()
    {
	m_streams.push_back(x::shared_ptr<FILE>(stderr, std::fclose));
    }
    void enable_file(const wchar_t *filename)
    {
	try {
	    FILE *fp = wfopenx(filename, L"w");
	    std::setbuf(fp, 0);
	    m_streams.push_back(x::shared_ptr<FILE>(fp, std::fclose));
	} catch (...) {}
    }
    void vprintf(const char *fmt, va_list args)
    {
	for (size_t i = 0; i < m_streams.size(); ++i)
	    std::vfprintf(m_streams[i].get(), fmt, args);
    }
    void printf(const char *fmt, ...)
    {
	va_list ap;
	va_start(ap, fmt);
	for (size_t i = 0; i < m_streams.size(); ++i)
	    std::vfprintf(m_streams[i].get(), fmt, ap);
	va_end(ap);
    }
};

inline void LOG(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    Log::instance()->vprintf(fmt, ap);
    va_end(ap);
}
