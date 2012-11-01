#include <cstdio>
#include <cstdarg>
#include <vector>
#include "util.h"

namespace util {
    void bswap16buffer(uint8_t *buffer, size_t size)
    {
	for (uint8_t *p = buffer; p < buffer + size; p += 2) {
	    uint8_t tmp = p[0];
	    p[0] = p[1];
	    p[1] = tmp;
	}
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
	for (uint8_t *p = buffer; p < buffer + size; p += 4) {
	    uint8_t tmp = p[0];
	    p[0] = p[3];
	    p[3] = tmp;
	    tmp = p[1];
	    p[1] = p[2];
	    p[2] = tmp;
	}
    }

    void bswap64buffer(uint8_t *buffer, size_t size)
    {
	for (uint8_t *p = buffer; p < buffer + size; p += 8) {
	    uint8_t tmp = p[0];
	    p[0] = p[7];
	    p[7] = tmp;
	    tmp = p[1];
	    p[1] = p[6];
	    p[6] = tmp;
	    tmp = p[2];
	    p[2] = p[5];
	    p[5] = tmp;
	    tmp = p[3];
	    p[3] = p[4];
	    p[4] = tmp;
	}
    }
}
