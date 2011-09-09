#include "riff.h"
#include <cassert>

void RIFFParser::parse()
{
    next();
    if (chunk_id() != 'RIFF' && chunk_id() != 'RF64')
	throw std::runtime_error("Not a RIFF file");
    set_format_id(down());
    if (chunk_id() == 'RF64')
	parse_datasize64();
}

bool RIFFParser::get_chunkinfo(uint32_t *fcc, uint64_t *size)
{
    char buff[4];
    if (stream().read(buff, 4) < 4)
	return false;
    *fcc = fourcc(buff);
    uint32_t x;
    check_eof(stream().read32le(&x));
    *size = x;
    // RF64 support
    chunk_map_t::const_iterator it = m_chunk_size_map.find(chunk_id());
    if (it != m_chunk_size_map.end())
	*size = it->second;
    return true;
}

void RIFFParser::parse_datasize64()
{
    next();
    if (chunk_id() != 'ds64')
	throw std::runtime_error("Invalid RF64 format");

    check_eof(stream().seek_forward(8) == 8);

    uint32_t x, y;
    check_eof(stream().read32le(&x));
    check_eof(stream().read32le(&y));
    m_chunk_size_map['data'] = ((static_cast<uint64_t>(y) << 32) | x);

    check_eof(stream().seek_forward(8) == 8);

    uint32_t len;
    check_eof(stream().read32le(&len));

    for (uint32_t i = 0; i < len; ++i) {
	char buff[4];
	check_eof(stream().read(buff, 4) == 4);
	check_eof(stream().read32le(&x));
	check_eof(stream().read32le(&y));
	m_chunk_size_map[fourcc(buff)]
	    = ((static_cast<uint64_t>(y) << 32) | x);
    }
    done_chunk();
}
