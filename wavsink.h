#ifndef _WAVESINK_H
#define _WAVESINK_H

#include "iointer.h"

class WaveSink : public ISink {
    FILE *m_file;
    bool m_closed;
    bool m_seekable;
    bool m_rf64;
    uint16_t m_bytes_per_frame;
    uint32_t m_chanmask;
    uint32_t m_data_pos;
    uint64_t m_bytes_written;
    AudioStreamBasicDescription m_asbd;
public:
    WaveSink(FILE *fp, uint64_t duration,
	     const AudioStreamBasicDescription &format,
	     uint32_t chanmask=0);
    ~WaveSink() { try { finishWrite(); } catch (...) {} }
    void writeSamples(const void *data, size_t length, size_t nsamples);
    void finishWrite();
private:
    template <typename T>
    void put(std::streambuf *os, T obj)
    {
	os->sputn(reinterpret_cast<char*>(&obj), sizeof obj);
    }
    std::string buildHeader();
    void write(const void *data, size_t length)
    {
	std::fwrite(data, 1, length, m_file);
	if (ferror(m_file))
	    util::throw_crt_error("fwrite()");
    }
};

#endif
