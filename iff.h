#ifndef _IFF_H
#define _IFF_H

#include <map>
#include "util.h"
#include "ioabst.h"

class IFFParser: public BinaryRead<IFFParser> {
    InputStream m_stream;
    uint32_t m_format_id, m_chunk_id;
    uint64_t m_chunk_size, m_chunk_off;
    int m_is_odd_chunk;
    // container stack. <fcc, endpos>
    std::vector<std::pair<uint32_t, uint64_t> > m_container_stack;
public:
    enum { kContainerEnd = 'CenD' };
    explicit IFFParser(InputStream &stream):
        m_stream(stream), m_format_id(0), m_chunk_id(0),
        m_chunk_size(0), m_chunk_off(0), m_is_odd_chunk(0)
    {}
    virtual ~IFFParser() {}
    virtual void parse();
    uint32_t format_id() { return m_format_id; }
    std::string format_name() { return fourcc(m_format_id).svalue; }
    uint32_t chunk_id() { return m_chunk_id; }
    std::string chunk_name() { return fourcc(m_chunk_id).svalue; }
    std::string chunk_path();
    uint64_t chunk_size() { return m_chunk_size - m_is_odd_chunk; }
    // go to next chunk.
    bool next();
    // read some raw data from current chunk.
    size_t read(void *buffer, size_t count);
    int64_t seek_forward(int64_t count);
    virtual bool is_container(uint32_t id) { 
        return id == 'FORM' || id == 'CAT ' || id == 'LIST';
    }
    /*
     * go down into the container chunk, and returns container's fcc.
     * affects the subsequent next() behavior.
     * if not called, next() skips the whole container.
     * otherwise, next() iterates the container children. 
     * can be called just after next(), and when current chunk is a container.
     */
    uint32_t down();
protected:
    InputStream &stream() { return m_stream; }
    virtual bool get_chunkinfo(uint32_t *fcc, uint64_t *size);
    void done_chunk() { m_chunk_off = m_chunk_size; }
    void set_format_id(uint32_t id) { m_format_id = id; }
private:
    void skip();
};

#endif
