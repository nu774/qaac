#include "iff.h"
#include <cassert>

void IFFParser::parse()
{
    next();
    if (chunk_id() != 'FORM')
        throw std::runtime_error("Not a IFF file");
    set_format_id(down());
}

std::string IFFParser::chunk_path()
{
    std::string result = "";;
    for (size_t i = 0; i < m_container_stack.size(); ++i)
        result += format("/%s", fourcc(m_container_stack[i].first).svalue);
    result += "/" + chunk_name();
    return result;
}

bool IFFParser::next()
{
    skip();
    if (m_container_stack.size()) {
        if (m_stream.tell() == m_container_stack.back().second) {
            /* Virtual(dummy) chunk to tell the end of container chunk,
             * for ease of parsing */
            m_container_stack.pop_back();
            m_chunk_id = fourcc(kContainerEnd);
            return true;
        }
    }
    if (!get_chunkinfo(&m_chunk_id, &m_chunk_size))
        return false;
    m_is_odd_chunk = static_cast<int>(m_chunk_size & 1);
    if (m_is_odd_chunk)
        ++m_chunk_size;
    return true;
}

bool IFFParser::get_chunkinfo(uint32_t *fcc, uint64_t *size)
{
    char buff[4];
    if (m_stream.read(buff, 4) < 4)
        return false;
    *fcc = fourcc(buff);
    uint32_t x;
    check_eof(m_stream.read32be(&x));
    *size = x;
    return true;
}

size_t IFFParser::read(void *buffer, size_t count)
{
    uint64_t count64 = static_cast<uint64_t>(count);
    count = static_cast<size_t>(std::min(count64, chunk_size() - m_chunk_off));
    if (count) {
        count = m_stream.read(buffer, count);
        m_chunk_off += count;
    }
    return count;
}

int64_t IFFParser::seek_forward(int64_t count)
{
    count = std::min(static_cast<uint64_t>(count), chunk_size() - m_chunk_off);
    count = m_stream.seek_forward(count);
    m_chunk_off += count;
    return count;
}

void IFFParser::skip()
{
    if (m_chunk_off < m_chunk_size) {
        uint64_t nskip = m_chunk_size - m_chunk_off;
        check_eof(m_stream.seek_forward(nskip) == nskip);
    }
    m_chunk_off = m_chunk_size = 0;
}

uint32_t IFFParser::down()
{
    assert(is_container(chunk_id()) && m_chunk_off == 0);
    char buff[4];
    check_eof(m_stream.read(buff, 4) == 4);
    uint32_t fcc = fourcc(buff);
    uint64_t endpos = m_stream.tell() + m_chunk_size - 4;
    if (!m_container_stack.size())
        endpos = -1;
    m_container_stack.push_back(std::make_pair(fcc, endpos));
    done_chunk();
    return fcc;
}
