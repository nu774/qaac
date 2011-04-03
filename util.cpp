#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>

#if defined(_MSC_VER) || defined(__MINGW32__)
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif

char *strsep(char **strp, const char *sep)
{
    char *tok, *s;

    if (!strp || !(tok = *strp))
	return 0;
    if (s = std::strpbrk(tok, sep)) {
	*s = 0;
	*strp = s + 1;
    } else
	*strp = 0;
    return tok;
}

std::string format(const char *fmt, ...)
{
    va_list args;
    std::vector<char> buffer(128);

    va_start(args, fmt);
    int rc = vsnprintf(&buffer[0], buffer.size(), fmt, args);
    va_end(args);
    if (rc >= 0 && rc < static_cast<int>(buffer.size()))
	return std::string(&buffer[0], &buffer[rc]);
#if defined(_MSC_VER) || defined(__MINGW32__) 
    va_start(args, fmt);
    rc = _vscprintf(fmt, args);
    va_end(args);
    if (rc < 0) {
	// format failed
	return "";
    }
#endif
    buffer.resize(rc + 1);
    va_start(args, fmt);
    rc = vsnprintf(&buffer[0], buffer.size(), fmt, args);
    va_end(args);
    return std::string(&buffer[0], &buffer[rc]);
}

#if defined(_MSC_VER) || defined(__MINGW32__)
std::wstring format(const wchar_t *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int rc = _vscwprintf(fmt, args);
    va_end(args);

    std::vector<wchar_t> buffer(rc + 1);

    va_start(args, fmt);
    rc = _vsnwprintf(&buffer[0], buffer.size(), fmt, args);
    va_end(args);

    return std::wstring(&buffer[0], &buffer[rc]);
}
#endif
