#include <cstdio>
#include <cstdarg>
#include <vector>
#include "util.h"

namespace util {
    void bswap16buffer(uint16_t *bp, size_t size)
    {
        for (uint16_t *endp = bp + size; bp != endp; ++bp)
            *bp = _byteswap_ushort(*bp);
    }

    void bswap24buffer(uint8_t *buffer, size_t size)
    {
        for (uint8_t *p = buffer; p < buffer + size; p += 3) {
            uint8_t tmp = p[0];
            p[0] = p[2];
            p[2] = tmp;
        }
    }

    void bswap32buffer(uint32_t *bp, size_t size)
    {
        for (uint32_t *endp = bp + size; bp != endp; ++bp)
            *bp = _byteswap_ulong(*bp);
    }

    void bswap64buffer(uint64_t *bp, size_t size)
    {
        for (uint64_t *endp = bp + size; bp != endp; ++bp)
            *bp = _byteswap_uint64(*bp);
    }

    void bswapbuffer(void *buffer, size_t size, uint32_t width)
    {
        switch (width) {
        case 16:
            bswap16buffer(static_cast<uint16_t*>(buffer), size / 2);
            break;
        case 24:
            bswap24buffer(static_cast<uint8_t*>(buffer), size);
            break;
        case 32:
            bswap32buffer(static_cast<uint32_t*>(buffer), size / 4);
            break;
        case 64:
            bswap64buffer(static_cast<uint64_t*>(buffer), size / 8);
            break;
        }
    }

    template <typename X, typename Y>
    void packXtoY(void *data, size_t count)
    {
        const X *src = static_cast<X*>(data);
        Y *dst = static_cast<Y*>(data);
        const int shifts = (sizeof(X) - sizeof(Y)) * 8;
        
        for (size_t i = 0; i < count; ++i)
            dst[i] = static_cast<Y>(src[i] >> shifts);
    }

    void pack(void *data, size_t *size, unsigned width, unsigned new_width)
    {
        if (width == new_width)
            return;
        else if (width == 4 && new_width == 1) {
            packXtoY<uint32_t, uint8_t>(data, *size / 4);
            *size /= 4;
        } else if (width == 4 && new_width == 2) {
            packXtoY<uint32_t, uint16_t>(data, *size / 4);
            *size /= 2;
        } else if (width == 4 && new_width == 3) {
            const uint8_t *src = static_cast<uint8_t*>(data);
            uint8_t *dst = static_cast<uint8_t*>(data);
            const size_t count = *size / 4;
            for (size_t i = 0; i < count; ++i) {
                dst[0] = src[1];
                dst[1] = src[2];
                dst[2] = src[3];
                src += 4;
                dst += 3;
            }
            *size = count * 3;
        } else {
            throw std::runtime_error("util::pack(): BUG");
        }
    }

    template <typename X, typename Y>
    void unpackXtoY(const X *src, Y *dst, size_t count)
    {
        const int shifts = (sizeof(Y) - sizeof(X)) * 8;
        for (size_t i = 0; i < count; ++i)
            dst[i] = static_cast<Y>(src[i] << shifts);
    }

    void unpack(const void *input, void *output, size_t *size, unsigned width,
                unsigned new_width)
    {
        if (width == new_width)
            std::memcpy(output, input, *size);
        else if (width == 1 && new_width == 4) {
            unpackXtoY(static_cast<const uint8_t *>(input),
                       static_cast<uint32_t *>(output), *size);
            *size *= 4;
        } else if (width == 2 && new_width == 4) {
            unpackXtoY(static_cast<const uint16_t *>(input),
                       static_cast<uint32_t *>(output), *size / 2);
            *size *= 2;
        } else if (width == 3 && new_width == 4) {
            const uint8_t *src = static_cast<const uint8_t*>(input);
            uint8_t *dst = static_cast<uint8_t*>(output);
            const size_t count = *size / 3;
            for (size_t i = 0; i < count; ++i) {
                dst[0] = '\0';
                dst[1] = src[0];
                dst[2] = src[1];
                dst[3] = src[2];
                src += 3;
                dst += 4;
            }
            *size = count * 4;
        } else {
            throw std::runtime_error("util::unpack(): BUG");
        }
    }

    void convert_sign(uint32_t *data, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
            data[i] ^= 0x80000000U;
    }

    ssize_t nread(int fd, void *buffer, size_t size)
    {
        char *bp = static_cast<char*>(buffer);
        size_t total = 0;
        ssize_t n = 0;
        while (total < size) {
            n = read(fd, bp, size - total);
            if (n <= 0)
                break;
            bp += n;
            total += n;
        }
        return total > 0 ? total : n;
    }

    bool parse_timespec(const char *spec, double sample_rate,
                        int64_t *result)
    {
        int hh, mm, s, ff, sign = 1;
        char a, b;
        double ss;
        if (!spec || !*spec)
            return false;
        if (std::sscanf(spec, "%lld%c%c", result, &a, &b) == 2 && a == 's')
            return true;
        if (spec[0] == '-') {
            sign = -1;
            ++spec;
        }
        if (std::sscanf(spec, "%d:%d:%d%c%c", &mm, &s, &ff, &a, &b) == 4 &&
            a == 'f')
            ss = mm * 60 + s + ff / 75.0;
        else if (std::sscanf(spec, "%d:%d:%lf%c", &hh, &mm, &ss, &a) == 3)
            ss = ss + ((hh * 60.0) + mm) * 60.0;
        else if (std::sscanf(spec, "%d:%lf%c", &mm, &ss, &a) == 2)
            ss = ss + mm * 60.0;
        else if (std::sscanf(spec, "%lf%c", &ss, &a) != 1)
            return false;

        *result = sign * static_cast<int64_t>(sample_rate * ss + .5);
        return true;
    }
}
