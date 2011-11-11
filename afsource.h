#ifndef AFSOURCE_H
#define AFSOURCE_H

#include "AudioFileX.h"

class AFSource: public ISource, public PartialSource<AFSource>
{
    AudioFileX m_af;
    SampleFormat m_format;
    std::vector<uint32_t> m_chanmap;
    uint64_t m_offset;
public:
    AFSource(const wchar_t *path);
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const
    {
	return m_chanmap.size() ? &m_chanmap: 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
};

#endif
