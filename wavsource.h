#ifndef _WAVSOURCE_H
#define _WAVSOURCE_H

#include <algorithm>
#include "iointer.h"
#include "riff.h"

class WaveSource : public ISource, private RIFFParser {
    SampleFormat m_format;
    std::vector<uint32_t> m_chanmap;
    uint64_t m_duration;
    uint64_t m_samples_read;
    bool m_ignore_length;
public:
    explicit WaveSource(InputStream &stream, bool ignorelength=false);
    void setRange(int64_t start=0, int64_t length=-1);
    uint64_t length() const { return m_duration; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const
    {
	return m_chanmap.size() ? &m_chanmap : 0;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	uint64_t rest = m_duration - m_samples_read;
	nsamples = static_cast<size_t>(
		std::min(static_cast<uint64_t>(nsamples), rest));
	if (!nsamples) return 0;
	size_t nblocks = m_format.bytesPerFrame();
	return readx(buffer, nsamples * nblocks) / nblocks;
    }
private:
    size_t readx(void *buffer, size_t count)
    {
	if (m_ignore_length)
	    return stream().read(buffer, count);
	else
	    return read(buffer, count);
    }
    int64_t seek_forwardx(int64_t count)
    {
	if (m_ignore_length)
	    return stream().seek_forward(count);
	else
	    return seek_forward(count);
    }
    void fetchWaveFormat();
    void parseInfo();
};

#endif
