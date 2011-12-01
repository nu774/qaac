#ifndef _WAVESINK_H
#define _WAVESINK_H

#include "iointer.h"

class WaveSink : public ISink {
    FILE *m_file;
    uint32_t m_data_pos;
    uint64_t m_bytes_written;
public:
    WaveSink(FILE *fp, uint64_t duration, const SampleFormat &format,
	    uint32_t chanmask=0);
    ~WaveSink() { finishWrite(); }
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
	write(data, length);
	m_bytes_written += length;
    }
    void finishWrite();
private:
    void write(const void *data, size_t length)
    {
	std::fwrite(data, 1, length, m_file);
	if (ferror(m_file))
	    throw std::runtime_error(std::strerror(errno));
    }
};

#endif
