#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <cstddef>
#include <vector>
#include <stdint.h>

class BitStream {
    std::vector<uint8_t> m_buffer;
    size_t m_cur, m_pos;
public:
    BitStream(): m_cur(0), m_pos(0)
    {}
    BitStream(const uint8_t *data, size_t size):
        m_buffer(data, data + size), m_cur(0), m_pos(0)
    {}
    size_t position() const { return (m_cur << 3) + m_pos; }
    const uint8_t *data() const { return &m_buffer[0]; }
    uint32_t peek(uint32_t nbits);
    uint32_t get(uint32_t nbits);
    void put(uint32_t value, uint32_t nbits);
    void advance(size_t nbits);
    void rewind() { m_cur = m_pos = 0; }
    void byteAlign() { if (m_pos) put(0, 8 - m_pos); }
    uint32_t copy(BitStream &src, uint32_t nbits)
    {
        uint32_t val = src.get(nbits);
        put(val, nbits);
        return val;
    }
};

#endif
