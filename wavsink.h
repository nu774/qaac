#ifndef _WAVESINK_H
#define _WAVESINK_H

#include "iointer.h"

class WavSink : public ISink {
    FILE *m_file;
public:
    WavSink(FILE *fp, uint64_t duration, const SampleFormat &format,
	    const std::vector<uint32_t> *chanmap=0);
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
	write(data, length);
    }
private:
    void write(const void *data, size_t length)
    {
	std::fwrite(data, 1, length, m_file);
	if (ferror(m_file))
	    throw std::runtime_error(std::strerror(errno));
    }
};

#endif
