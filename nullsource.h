#ifndef _NULLSOURCE_H
#define _NULLSOURCE_H

#include "iointer.h"

class NullSource: public ISource, public PartialSource<NullSource> {
    SampleFormat m_format;
public:
    NullSource(const SampleFormat &format):
	m_format(format)
    {}
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	nsamples = adjustSamplesToRead(nsamples);
	if (nsamples) {
	    size_t nblocks = m_format.bytesPerFrame();
	    std::memset(buffer, nsamples * nblocks, 0);
	    addSamplesRead(nsamples);
	}
	return nsamples;
    }
    void skipSamples(int64_t count) {}
};

#endif
