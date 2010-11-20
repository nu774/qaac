#ifndef _NULLSOURCE_H
#define _NULLSOURCE_H

#include "iointer.h"

class NullSource: public ISource {
    SampleFormat m_format;
    uint64_t m_duration, m_samples_read;
public:
    NullSource(const SampleFormat &format):
	m_format(format), m_duration(0), m_samples_read(0)
    {}
    void setRange(int64_t start=0, int64_t length=-1)
    {
	m_duration = length - start;
    }
    uint64_t length() const { return m_duration; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	size_t rest = static_cast<size_t>(m_duration - m_samples_read);
	nsamples = std::min(nsamples, rest);
	if (nsamples) {
	    size_t nblocks = m_format.bytesPerFrame();
	    std::memset(buffer, nsamples * nblocks, 0);
	    m_samples_read += nsamples;
	}
	return nsamples;
    }
};

#endif
