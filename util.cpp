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
}
