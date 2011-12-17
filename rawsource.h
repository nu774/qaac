#ifndef _RAWSOURCE_H
#define _RAWSOURCE_H

#include "iointer.h"

class RawSource: public ISource, public PartialSource<RawSource> {
    InputStream m_stream;
    SampleFormat m_format;
public:
    explicit RawSource(InputStream &stream, const SampleFormat &format)
	: m_stream(stream), m_format(format)
    {
	int64_t len = m_stream.size();
	setRange(0, len == -1 ? -1 : len / format.bytesPerFrame());
    }
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	nsamples = adjustSamplesToRead(nsamples);
	if (nsamples) {
	    size_t nblocks = m_format.bytesPerFrame();
	    nsamples = m_stream.read(buffer, nsamples * nblocks) / nblocks;
	    addSamplesRead(nsamples);
	}
	return nsamples;
    }
    void skipSamples(int64_t count)
    {
	int64_t bytes = count * m_format.bytesPerFrame();
	if (m_stream.seek_forward(bytes) != bytes)
	    throw std::runtime_error("RawSource: seek failed");
    }
};

#endif
