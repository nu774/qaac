#ifndef _RIFF_H
#define _RIFF_H

#include "iff.h"

class RIFFParser: public IFFParser {
    typedef std::map<uint32_t, uint64_t> chunk_map_t;
    // chunk size mapping for RF64 format. <fcc, size>
    chunk_map_t m_chunk_size_map;
public:
    explicit RIFFParser(InputStream &stream): IFFParser(stream) {}
    virtual ~RIFFParser() {}
    virtual void parse();
    virtual bool is_container(uint32_t id) { 
	return id == 'RIFF' || id == 'RF64' || id == 'LIST';
    }
protected:
    virtual bool get_chunkinfo(uint32_t *fcc, uint64_t *size);
private:
    void parse_datasize64();
};

#endif
