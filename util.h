#ifndef _UTIL_H
#define _UTIL_H

#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <cerrno>
#include <stdint.h>
#include <sys/stat.h>
#include <io.h>
#include "strutil.h"

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

#ifdef _MSC_VER
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

namespace util {
    template <typename T, size_t size>
    inline size_t sizeof_array(const T (&)[size]) { return size; }

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

    inline void *xcalloc(size_t count, size_t size)
    {
        void *memory = std::calloc(count, size);
        if (!memory) throw std::bad_alloc();
        return memory;
    }

    inline bool is_seekable(int fd)
    {
        struct stat stb = { 0 };
        return fstat(fd, &stb) == 0 && (stb.st_mode & S_IFMT) == S_IFREG;
    }

    template <typename ForwardIterator>
    bool is_increasing(ForwardIterator begin, ForwardIterator end)
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

    inline
    uint32_t bitcount(uint32_t bits)
    {
        bits = (bits & 0x55555555) + (bits >> 1 & 0x55555555);
        bits = (bits & 0x33333333) + (bits >> 2 & 0x33333333);
        bits = (bits & 0x0f0f0f0f) + (bits >> 4 & 0x0f0f0f0f);
        bits = (bits & 0x00ff00ff) + (bits >> 8 & 0x00ff00ff);
        return (bits & 0x0000ffff) + (bits >>16 & 0x0000ffff);
    }

    inline uint16_t l2host16(uint16_t n) { return n; }
    inline uint32_t l2host32(uint32_t n) { return n; }
    inline uint64_t l2host64(uint64_t n) { return n; }

    inline uint16_t b2host16(uint16_t n)
    {
        return _byteswap_ushort(n);
    }
    inline uint32_t b2host32(uint32_t n)
    {
        return _byteswap_ulong(n);
    }
    inline uint64_t b2host64(uint64_t n)
    {
        return _byteswap_uint64(n);
    }
    inline uint32_t h2big32(uint32_t n)
    {
        return _byteswap_ulong(n);
    }

    void bswapbuffer(uint8_t *buffer, size_t size, uint32_t width);

    inline void throw_crt_error(const std::string &message)
    {
        std::stringstream ss;
        ss << message << ": " << std::strerror(errno);
        throw std::runtime_error(ss.str());
    }

    inline void throw_crt_error(const std::wstring &message)
    {
        std::stringstream ss;
        ss << strutil::w2us(message) << ": " << std::strerror(errno);
        throw std::runtime_error(ss.str());
    }

    class FilePositionSaver
    {
    private:
        int m_fd;
        int64_t m_saved_position;
    public:
        explicit FilePositionSaver(int fd): m_fd(fd)
        {
            m_saved_position = _lseeki64(m_fd, 0, SEEK_CUR);
        }
        ~FilePositionSaver()
        {
            _lseeki64(m_fd, m_saved_position, SEEK_SET);
        }
    };

    void pack(void *data, size_t *size, unsigned width, unsigned new_width);

    void unpack(const void *input, void *output, size_t *size, unsigned width,
                unsigned new_width);

    ssize_t nread(int fd, void *buffer, size_t size);
}

#define CHECKCRT(expr) \
    do { \
        if (expr) { \
            util::throw_crt_error(#expr); \
        } \
    } while (0)

#endif
