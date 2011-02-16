#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <algorithm>
#include <iterator>
#include <stdexcept>

#if defined _MSC_VER
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#endif

#if !defined(_MSC_VER) && !defined(__MINGW32__)
inline int _wtoi(const wchar_t *s) { return std::wcstol(s, 0, 10); }
#endif

template <typename T, size_t size>
inline size_t array_size(const T (&)[size]) { return size; }

template <typename T, typename U>
class find_string_pred {
    T m_value; 
public:
    find_string_pred(T value): m_value(value) {}
    bool operator()(U ch) { return !ch || ch == m_value; }
};

template <typename T, typename U>
inline int strindex(const T *s, U ch)
{
    const T *p = std::find_if(s, reinterpret_cast<const T*>(-1LL),
	    find_string_pred<U, T>(ch));
    return *p ? p - s : -1;
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

char *strsep(char **stringp, const char *delim);

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

inline
bool big_endian_host()
{
    int n = 1;
    return !reinterpret_cast<char*>(&n);
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
#endif
