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

    void pack(uint8_t *data, size_t *size, unsigned width, unsigned new_width)
    {
	unsigned diff = width - new_width;
	uint8_t *dst = data, *src = data + diff;
	size_t limit = *size / width;
	for (size_t i = 0; i < limit; ++i) {
	    std::memmove(dst, src, new_width);
	    dst += new_width;
	    src += width;
	}
	*size -= limit * diff;
    }
}
