#ifndef _NULLSOURCE_H
#define _NULLSOURCE_H

#include "iointer.h"

class NullSource: public PartialSource<NullSource> {
    AudioStreamBasicDescription m_asbd;
public:
    NullSource(const AudioStreamBasicDescription &asbd):
	m_asbd(asbd)
    {}
    uint64_t length() const { return getDuration(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	nsamples = adjustSamplesToRead(nsamples);
	if (nsamples) {
	    size_t nblocks = m_asbd.mBytesPerFrame;
	    std::memset(buffer, 0, nsamples * nblocks);
	    addSamplesRead(nsamples);
	}
	return nsamples;
    }
    void skipSamples(int64_t count) {}
};

#endif
