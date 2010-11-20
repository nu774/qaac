#ifndef _RAWSOURCE_H
#define _RAWSOURCE_H

#include "iointer.h"

class RawSource: public ISource {
    InputStream m_stream;
    SampleFormat m_format;
    uint64_t m_duration;
    uint64_t m_samples_read;
public:
    explicit RawSource(InputStream &stream, const SampleFormat &format)
	: m_stream(stream), m_format(format), m_samples_read(0)
    {
	m_duration = m_stream.size();
	if (m_duration != -1) m_duration /= format.bytesPerFrame();
    }
    void setRange(int64_t start=0, int64_t length=-1)
    {
	if (start > 0) {
	    int64_t bytes = start * m_format.bytesPerFrame();
	    if (m_stream.seek_forward(bytes) != bytes)
		throw std::runtime_error("RawSource: seek failed");
	}
	int64_t dur = m_duration;
	if (length >= 0 && (dur == -1 || length < dur))
	    m_duration = length;
    }
    uint64_t length() const { return m_duration; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	uint64_t rest = m_duration - m_samples_read;
	nsamples = static_cast<size_t>(
		std::min(static_cast<uint64_t>(nsamples), rest));
	if (!nsamples) return 0;
	size_t nblocks = m_format.bytesPerFrame();
	size_t rc = m_stream.read(buffer, nsamples * nblocks) / nblocks;
	m_samples_read += rc;
	return rc;
    }
};

#endif
