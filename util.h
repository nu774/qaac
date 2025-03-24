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
#include <memory>
#include <stdint.h>
#include <sys/stat.h>
#include <io.h>
#include "strutil.h"
#include "IInputStream.h"

#if defined(_MSC_VER) && _MSC_VER < 1800
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

    template <typename T> class FIFO {
        std::vector<T> m_data;
        size_t m_unit, m_begin, m_end;
    public:
        FIFO(): m_data(256), m_unit(1), m_begin(0), m_end(0) {}
        void set_unit(size_t n) { m_unit = n; }
        void reset() { m_begin = m_end = 0; }
        size_t count() { return (m_end - m_begin) / m_unit; }
        T *read_ptr() { return &m_data[m_begin]; }
        void advance(size_t n) { m_begin += n * m_unit; }
        T *read(size_t n)
        {
            T *begin = read_ptr();
            advance(n);
            return begin;
        }
        T *write_ptr() { return &m_data[m_end]; }
        void commit(size_t n) { m_end += n * m_unit; }
        void reserve(size_t n)
        {
            if (m_begin == m_end)
                reset();
            if (m_begin > 0) {
                std::memmove(&m_data[0], read_ptr(),
                             sizeof(T) * (m_end - m_begin));
                m_end -= m_begin;
                m_begin = 0;
            }
            if (m_end + n * m_unit > m_data.size()) {
                m_data.resize(m_end + n * m_unit);
            }
        }
    };

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

    void bswapbuffer(void *buffer, size_t size, uint32_t width);

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
        std::shared_ptr<IInputStream> m_stream;
        int64_t m_saved_position;
    public:
        explicit FilePositionSaver(std::shared_ptr<IInputStream> stream)
        : m_stream(stream)
        {
            m_saved_position = m_stream->tell();
        }
        ~FilePositionSaver()
        {
            m_stream->seek(m_saved_position, SEEK_SET);
        }
    };

    void pack(void *data, size_t *size, unsigned width, unsigned new_width);

    void unpack(const void *input, void *output, size_t *size, unsigned width,
                unsigned new_width);

    void convert_sign(uint32_t *data, size_t size);

    ssize_t nread(int fd, void *buffer, size_t size);

    inline double dB_to_scale(double dB)
    {
        return std::pow(10, 0.05 * dB);
    }

    inline double scale_to_dB(double scale)
    {
        return 20 * std::log10(scale);
    }

    bool parse_timespec(const wchar_t *spec, double sample_rate,
                        int64_t *result);

    inline void seconds_to_HMS(double seconds, int *h, int *m, int *s,
                               int *millis)
    {
        *h = seconds / 3600;
        seconds -= *h * 3600;
        *m = seconds / 60;
        seconds -= *m * 60;
        *s = seconds;
        *millis = (seconds - *s) * 1000;
    }

    inline std::wstring format_seconds(double seconds)
    {
        int h, m, s, millis;
        seconds_to_HMS(seconds, &h, &m, &s, &millis);
        return h ? strutil::format(L"%d:%02d:%02d.%03d", h, m, s, millis)
                 : strutil::format(L"%d:%02d.%03d", m, s, millis);
    }
}

#define CHECKCRT(expr) \
    do { \
        if (expr) { \
            util::throw_crt_error(#expr); \
        } \
    } while (0)

#endif
