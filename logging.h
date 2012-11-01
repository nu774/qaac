#include <cstdio>
#include <cstdarg>
#include <vector>
#include "ioabst.h"

/* non-thread safe */
class Log {
    std::vector<std::shared_ptr<FILE> > m_streams;
    static Log *m_instance;
    Log() {}
public:
    static Log *instance()
    {
	if (!m_instance) m_instance = new Log();
	return m_instance;
    }
    bool is_enabled() { return m_streams.size() != 0; }
    void enable_stderr()
    {
	m_streams.push_back(std::shared_ptr<FILE>(stderr, std::fclose));
    }
    void enable_file(const wchar_t *filename)
    {
	try {
	    FILE *fp = wfopenx(filename, L"w");
#ifdef _MSC_VER
	    _setmode(_fileno(fp), _O_U8TEXT);
#endif
	    std::setbuf(fp, 0);
	    m_streams.push_back(std::shared_ptr<FILE>(fp, std::fclose));
	} catch (...) {}
    }
    void vwprintf(const wchar_t *fmt, va_list args)
    {
	for (size_t i = 0; i < m_streams.size(); ++i)
	    std::vfwprintf(m_streams[i].get(), fmt, args);
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
