#ifndef _NULLSOURCE_H
#define _NULLSOURCE_H

#include "iointer.h"

class NullSource: public PartialSource<NullSource> {
    AudioStreamBasicDescription m_format;
public:
    NullSource(const AudioStreamBasicDescription &format):
	m_format(format)
    {}
    uint64_t length() const { return getDuration(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_format;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	nsamples = adjustSamplesToRead(nsamples);
	if (nsamples) {
	    size_t nblocks = m_format.mBytesPerFrame;
	    std::memset(buffer, 0, nsamples * nblocks);
	    addSamplesRead(nsamples);
	}
	return nsamples;
    }
    void skipSamples(int64_t count) {}
};

#endif
