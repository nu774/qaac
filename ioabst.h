#ifndef _CHANNEL_H
#define _CHANNEL_H
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <vector>
#include <stdint.h>
#include "util.h"
#include "strutil.h"
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <io.h>
#include "win32util.h"
#endif

template <class T>
class BinaryRead {
public:
    bool read16le(uint16_t *result)
    {
	if (((T*)(this))->read(result, 2) != 2)
	    return false;
	*result = util::l2host16(*result);
	return true;
    }
    bool read16be(uint16_t *result)
    {
	if (((T*)(this))->read(result, 2) != 2)
	    return false;
	*result = util::b2host16(*result);
	return true;
    }
    bool read32le(uint32_t *result)
    {
	if (((T*)(this))->read(result, 4) != 4)
	    return false;
	*result = util::l2host32(*result);
	return true;
    }
    bool read32be(uint32_t *result)
    {
	if (((T*)(this))->read(result, 4) != 4)
	    return false;
	*result = util::b2host32(*result);
	return true;
    }
};

class MemoryReader: public BinaryRead<MemoryReader>
{
    const uint8_t *m_position;
    const uint8_t *m_end;
public:
    MemoryReader(const void *beg, size_t size)
    {
	m_position = static_cast<const uint8_t*>(beg);
	m_end = m_position + size;
    }
    MemoryReader(const void *beg, const void *end)
    {
	m_position = static_cast<const uint8_t*>(beg);
	m_position = static_cast<const uint8_t*>(end);
    }
    ssize_t read(void *buffer, size_t count)
    {
	size_t n = std::min(m_end - m_position,
			    static_cast<ptrdiff_t>(count));
	std::memcpy(buffer, m_position, n);
	m_position += n;
	return n;
    }
    size_t skip(size_t count)
    {
	size_t n = std::min(m_end - m_position,
			    static_cast<ptrdiff_t>(count));
	m_position += n;
	return n;
    }
};

#endif
