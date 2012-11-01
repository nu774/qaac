#include <stdarg.h>
#include "strutil.h"

namespace strutil {
    template<> char *strsep(char **strp, const char *sep)
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
    template<> wchar_t *strsep(wchar_t **strp, const wchar_t *sep)
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

    std::wstring &m2w(std::wstring &dst, const char *src, size_t srclen,
	    const std::codecvt<wchar_t, char, std::mbstate_t> &cvt)
    {
	typedef std::codecvt<wchar_t, char, std::mbstate_t> cvt_t;
	wchar_t buffer[0x100];
	std::mbstate_t state = { 0 };
	const char *pend = src + srclen, *pnext = src;
	wchar_t *pwbegin = buffer,
		*pwend = buffer + sizeof(buffer)/sizeof(buffer[0]),
		*pwnext = pwbegin;
	dst.clear();
	while (src < pend) {
	    if (cvt.in(state, src, pend, pnext,
			pwbegin, pwend, pwnext) == cvt_t::error)
		throw std::runtime_error("conversion failed");
	    dst.append(pwbegin, pwnext - pwbegin);
	    pwnext = pwbegin;
	    src = pnext;
	}
	return dst;
    }

    std::string &w2m(std::string &dst, const wchar_t *src, size_t srclen,
		   const std::codecvt<wchar_t, char, std::mbstate_t> &cvt)
    {
	typedef std::codecvt<wchar_t, char, std::mbstate_t> cvt_t;
	char buffer[0x100];
	std::mbstate_t state = { 0 };
	const wchar_t *pwend = src + srclen, *pwnext = src;
	char *pbegin = buffer,
	     *pend = buffer + sizeof(buffer),
	     *pnext = pbegin;
	dst.clear();
	while (src < pwend) {
	    if (cvt.out(state, src, pwend, pwnext,
			pbegin, pend, pnext) == cvt_t::error)
		throw std::runtime_error("conversion failed");
	    dst.append(pbegin, pnext - pbegin);
	    pnext = pbegin;
	    src = pwnext;
	}
	return dst;
    }

#if defined(_MSC_VER) || defined(__MINGW32__)
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif
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
}
