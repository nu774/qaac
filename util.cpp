#include <cstdio>
#include <cstdarg>
#include <vector>
#include "util.h"

namespace util {
    void bswap16buffer(uint8_t *buffer, size_t size)
    {
        uint16_t *bp = reinterpret_cast<uint16_t*>(buffer);
        for (uint16_t *endp = bp + size / 2; bp != endp; ++bp)
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

    void bswap32buffer(uint8_t *buffer, size_t size)
    {
        uint32_t *bp = reinterpret_cast<uint32_t*>(buffer);
        for (uint32_t *endp = bp + size / 4; bp != endp; ++bp)
            *bp = _byteswap_ulong(*bp);
    }

    void bswap64buffer(uint8_t *buffer, size_t size)
    {
        uint64_t *bp = reinterpret_cast<uint64_t*>(buffer);
        for (uint64_t *endp = bp + size / 8; bp != endp; ++bp)
            *bp = _byteswap_uint64(*bp);
    }

    void bswapbuffer(uint8_t *buffer, size_t size, uint32_t width)
    {
        switch (width) {
        case 16:
            bswap16buffer(buffer, size);
            break;
        case 24:
            bswap24buffer(buffer, size);
            break;
        case 32:
            bswap32buffer(buffer, size);
            break;
        case 64:
            bswap64buffer(buffer, size);
            break;
        }
    }

    template <typename X, typename Y>
    void packXtoY(void *data, size_t *size)
    {
        const X *src = static_cast<X*>(data);
        Y *dst = static_cast<Y*>(data);
        const int count = static_cast<int>(*size / sizeof(X));
        const int shifts = (sizeof(X) - sizeof(Y)) * 8;
        
        for (int i = 0; i < count; ++i)
            dst[i] = static_cast<Y>(src[i] >> shifts);
        *size = count * sizeof(Y);
    }

    void pack(void *data, size_t *size, unsigned width, unsigned new_width)
    {
        if (width == new_width)
            return;
        else if (width == 4 && new_width == 1)
            packXtoY<uint32_t, uint8_t>(data, size);
        else if (width == 4 && new_width == 2)
            packXtoY<uint32_t, uint16_t>(data, size);
        else if (width == 4 && new_width == 3) {
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
    void unpackXtoY(const void *input, void *output, size_t *size)
    {
        const X *src = static_cast<const X*>(input);
        Y *dst = static_cast<Y*>(output);
        const int count = static_cast<int>(*size / sizeof(X));
        const int shifts = (sizeof(Y) - sizeof(X)) * 8;
        
        for (int i = 0; i < count; ++i)
            dst[i] = static_cast<Y>(src[i] << shifts);
        *size = count * sizeof(Y);
    }

    void unpack(const void *input, void *output, size_t *size, unsigned width,
                unsigned new_width)
    {
        if (width == new_width)
            std::memcpy(output, input, *size);
        else if (width == 1 && new_width == 4)
            unpackXtoY<uint8_t, uint32_t>(input, output, size);
        else if (width == 2 && new_width == 4)
            unpackXtoY<uint16_t, uint32_t>(input, output, size);
        else if (width == 3 && new_width == 4) {
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
}
