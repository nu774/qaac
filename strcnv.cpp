#include "strcnv.h"

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
