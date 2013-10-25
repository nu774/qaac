#include <algorithm>
#include "bitstream.h"

uint32_t BitStream::peek(uint32_t nbits)
{
    uint8_t *p = &m_buffer[m_cur];
    uint32_t v = (*p++ << m_pos) & 0xff;
    if (nbits <= 8 - m_pos)
        return v >> (8 - nbits);
    v >>= m_pos;
    nbits = nbits - 8 + m_pos;
    for (; nbits >= 8; nbits -= 8)
        v = v << 8 | *p++;
    if (nbits > 0)
        v = v << nbits | (*p << nbits) >> 8;
    return v;
}

uint32_t BitStream::get(uint32_t nbits)
{
    uint32_t value = peek(nbits);
    advance(nbits);
    return value;
}

void BitStream::put(uint32_t value, uint32_t nbits)
{
    uint32_t free_bits = 8 - m_pos;
    while (nbits > 0) {
        uint32_t width = std::min(free_bits, nbits);
        uint32_t new_free_bits = free_bits - width;
        uint32_t v = value >> (nbits - width);
        uint32_t mask = 0xffu >> (8 - width);
        mask <<= new_free_bits;
        v = (v << new_free_bits) & mask;
        while (m_buffer.size() <= m_cur)
            m_buffer.push_back(0);
        m_buffer[m_cur] = (m_buffer[m_cur] & ~mask) | v;
        nbits -= width;
        free_bits = new_free_bits;
        if (free_bits == 0) {
            ++m_cur;
            free_bits = 8;
        }
    }
    m_pos = 8 - free_bits;
}

void BitStream::advance(size_t nbits)
{
    if (nbits) {
        m_pos += nbits;
        m_cur += (m_pos >> 3);
        m_pos &= 7;
    }
}
