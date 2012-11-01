#ifndef _RAWSOURCE_H
#define _RAWSOURCE_H

#include "iointer.h"

class RawSource: public PartialSource<RawSource> {
    InputStream m_stream;
    AudioStreamBasicDescription m_asbd;
public:
    RawSource(InputStream &stream, const AudioStreamBasicDescription &asbd)
	: m_stream(stream), m_asbd(asbd)
    {
	int64_t len = m_stream.size();
	setRange(0, len == -1 ? -1 : len / asbd.mBytesPerFrame);
    }
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
	    nsamples = m_stream.read(buffer, nsamples * nblocks) / nblocks;
	    addSamplesRead(nsamples);
	}
	return nsamples;
    }
    void skipSamples(int64_t count)
    {
	int64_t bytes = count * m_asbd.mBytesPerFrame;
	if (m_stream.seek_forward(bytes) != bytes)
	    throw std::runtime_error("RawSource: seek failed");
    }
};

#endif
