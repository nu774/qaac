#ifndef _UTIL_H
#define _UTIL_H

#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <cerrno>
#include <stdint.h>
#include "strcnv.h"
#include "utf8_codecvt_facet.hpp"

#if defined _MSC_VER
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#endif

#ifdef _MSC_VER
typedef intptr_t ssize_t;
#endif

#ifdef _MSC_VER
#ifdef _M_IX86
inline int lrint(double x)
{
    int n;
    _asm {
	fld x
	fistp n
    }
    return n;
}
#else
#include <emmintrin.h>
inline int lrint(double x)
{
    return _mm_cvtsd_si32(_mm_load_sd(&x));
}
#endif
#endif

#if !defined(_MSC_VER) && !defined(__MINGW32__)
inline int _wtoi(const wchar_t *s) { return std::wcstol(s, 0, 10); }
#endif

template <typename T, size_t size>
inline size_t array_size(const T (&)[size]) { return size; }

inline ssize_t strindex(const char *s, int ch)
{
    const char *p = std::strchr(s, ch);
    return p ? p - s : -1;
}

inline ssize_t strindex(const wchar_t *s, int ch)
{
    const wchar_t *p = std::wcschr(s, ch);
    return p ? p - s : -1;
}

template <typename T>
inline void squeeze(T *str, const T *charset)
{
    T *q = str;
    for (T *p = str; *p; ++p)
	if (strindex(charset, *p) == -1)
	    *q++ = *p;
    *q = 0;
}

/* string conversion for ASCII only characters */
struct nallow_ {
    char operator()(wchar_t ch) { return static_cast<char>(ch); }
};
inline std::string nallow(const std::wstring& s)
{
    std::string result;
    std::transform(s.begin(), s.end(), std::back_inserter(result), nallow_());
    return result;
}
inline std::wstring widen(const std::string& s)
{
    std::wstring result;
    std::copy(s.begin(), s.end(), std::back_inserter(result));
    return result;
}

template <typename T>
std::basic_string<T> normalize_crlf(const T *s, const T *delim)
{
    std::basic_string<T> result;
    const T *p;
    T c;
    while ((c = *s++)) {
	if (c == '\r') {
	    for (p = delim; *p; ++p)
		result.push_back(*p);
	    if (*s == '\n')
		++s;
	}
	else if (c == '\n')
	    for (p = delim; *p; ++p)
		result.push_back(*p);
	else
	    result.push_back(c);
    }
    return result;
}

struct fourcc {
    uint32_t nvalue;
    char svalue[5];
    explicit fourcc(uint32_t v) : nvalue(v)
    {
	for (int i = 3; i >= 0; --i, v >>= 8)
	    svalue[i] = v & 0xff;
	svalue[4] = 0;
    }
    explicit fourcc(const char *s) : nvalue(0)
    {
	std::memcpy(svalue, s, 4);
	svalue[4] = 0;
	const unsigned char *p = reinterpret_cast<const unsigned char *>(s);
	nvalue = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    }
    operator const char *() const { return svalue; }
    operator uint32_t() const { return nvalue; }
};

inline
void *xmalloc(size_t size)
{
    void *memory = std::malloc(size);
    if (!memory) throw std::bad_alloc();
    return memory;
}

inline
void *xcalloc(size_t count, size_t size)
{
    void *memory = std::calloc(count, size);
    if (!memory) throw std::bad_alloc();
    return memory;
}

#ifndef _BSD_SOURCE
char *strsep(char **stringp, const char *delim);
#endif
wchar_t *wcssep(wchar_t **stringp, const wchar_t *delim);

std::string format(const char *fmt, ...);
#if defined(_MSC_VER) || defined(__MINGW32__)
std::wstring format(const wchar_t *fmt, ...);
#endif

template <typename T, typename Conv>
inline
std::basic_string<T> strtransform(const std::basic_string<T> &s, Conv conv)
{
    std::basic_string<T> result;
    std::transform(s.begin(), s.end(), std::back_inserter(result), conv);
    return result;
}

inline
std::wstring wsupper(const std::wstring &s)
{
    return strtransform(s, towupper);
}

inline
std::wstring wslower(const std::wstring &s)
{
    return strtransform(s, towlower);
}

inline
int tolower_(int ch)
{
    return tolower(ch);
}

inline
std::string slower(const std::string &s)
{
    return strtransform(s, tolower_);
}

template <typename ForwardIterator>
bool is_strict_ordered(ForwardIterator begin, ForwardIterator end)
{
    if (begin == end)
	return true;
    for (ForwardIterator it; it = begin, ++begin != end; )
	if (*it >= *begin)
	    return false;
    return true;
}

template <typename T>
class AutoDynaCast {
    T *m_pointer;
public:
    AutoDynaCast(T *p): m_pointer(p) {}
    template <typename U>
    operator U*() { return dynamic_cast<U*>(m_pointer); }
};

inline void check_eof(bool expr)
{
    if (!expr) throw std::runtime_error("Premature EOF");
}

class MemorySink8 {
    char *m_ptr;
public:
    MemorySink8(void *ptr): m_ptr(reinterpret_cast<char*>(ptr)) {}
    void put(uint32_t value) { *m_ptr++ = value; }
};

class MemorySink16LE {
    char *m_ptr;
public:
    MemorySink16LE(void *ptr): m_ptr(reinterpret_cast<char*>(ptr)) {}
    void put(uint32_t value)
    {
	*m_ptr++ = value;
	*m_ptr++ = value >> 8;
    }
};

class MemorySink24LE {
    char *m_ptr;
public:
    MemorySink24LE(void *ptr): m_ptr(reinterpret_cast<char*>(ptr)) {}
    void put(uint32_t value)
    {
	*m_ptr++ = value;
	*m_ptr++ = value >> 8;
	*m_ptr++ = value >> 16;
    }
};

class MemorySink32LE {
    char *m_ptr;
public:
    MemorySink32LE(void *ptr): m_ptr(reinterpret_cast<char*>(ptr)) {}
    void put(uint32_t value)
    {
	*m_ptr++ = value;
	*m_ptr++ = value >> 8;
	*m_ptr++ = value >> 16;
	*m_ptr++ = value >> 24;
    }
};

inline
uint32_t bitcount(uint32_t bits)
{
    bits = (bits & 0x55555555) + (bits >> 1 & 0x55555555);
    bits = (bits & 0x33333333) + (bits >> 2 & 0x33333333);
    bits = (bits & 0x0f0f0f0f) + (bits >> 4 & 0x0f0f0f0f);
    bits = (bits & 0x00ff00ff) + (bits >> 8 & 0x00ff00ff);
    return (bits & 0x0000ffff) + (bits >>16 & 0x0000ffff);
}

/* XXX: assumes little endian host */
inline uint16_t l2host16(uint16_t n) { return n; }
inline uint32_t l2host32(uint32_t n) { return n; }

inline uint16_t b2host16(uint16_t n)
{
    return (n >> 8) | (n << 8);
}
inline uint32_t b2host32(uint32_t n)
{
    return (b2host16(n & 0xffff) << 16) | b2host16(n >> 16);
}
inline uint64_t b2host64(uint64_t n)
{
    return (static_cast<uint64_t>(b2host32(n & 0xffffffff)) << 32) |
	    b2host32(n >> 32);
}

void bswap16buffer(uint8_t *buffer, size_t size);

void bswap16buffer(uint8_t *buffer, size_t size);

void bswap24buffer(uint8_t *buffer, size_t size);

void bswap32buffer(uint8_t *buffer, size_t size);

void bswap64buffer(uint8_t *buffer, size_t size);

inline void throw_crt_error(const std::wstring &message)
{
    std::wstring s = format(L"%s: %s", message.c_str(),
			    widen(std::strerror(errno)).c_str());
    throw std::runtime_error(w2m(s, utf8_codecvt_facet()));
}
inline void throw_crt_error(const std::string &message)
{
    throw std::runtime_error(format("%s: %s", message.c_str(),
				    std::strerror(errno)));
}
#define CHECKCRT(expr) \
    do { \
	if (expr) { \
	    throw_crt_error(#expr); \
	} \
    } while (0)
#endif
