#include <cstdarg>
#include <vector>
#include "util.h"

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

wchar_t *wcssep(wchar_t **strp, const wchar_t *sep)
{
    wchar_t *tok, *s;

    if (!strp || !(tok = *strp))
	return 0;
    if (s = std::wcspbrk(tok, sep)) {
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

void bswap16buffer(uint8_t *buffer, size_t size)
{
    for (uint8_t *p = buffer; p < buffer + size; p += 2) {
	uint8_t tmp = p[0];
	p[0] = p[1];
	p[1] = tmp;
    }
}

void bswap24buffer(uint8_t *buffer, size_t size)
{
    for (uint8_t *p = buffer; p < buffer + size; p += 3) {
	uint8_t tmp = p[0];
	p[0] = p[2];
	p[2] = tmp;
    }
}

void bswap32buffer(uint8_t *buffer, size_t size)
{
    for (uint8_t *p = buffer; p < buffer + size; p += 4) {
	uint8_t tmp = p[0];
	p[0] = p[3];
	p[3] = tmp;
	tmp = p[1];
	p[1] = p[2];
	p[2] = tmp;
    }
}

void bswap64buffer(uint8_t *buffer, size_t size)
{
    for (uint8_t *p = buffer; p < buffer + size; p += 4) {
	uint8_t tmp = p[0];
	p[0] = p[7];
	p[7] = tmp;
	tmp = p[1];
	p[1] = p[6];
	p[6] = tmp;
	tmp = p[2];
	p[2] = p[5];
	p[5] = tmp;
	tmp = p[3];
	p[3] = p[4];
	p[4] = tmp;
    }
}
