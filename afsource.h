#ifndef AFSOURCE_H
#define AFSOURCE_H

#include "AudioFileX.h"
#include "channel.h"

class AFSource: public ISource, public PartialSource<AFSource>
{
    AudioFileX m_af;
    uint64_t m_offset;
    std::vector<uint32_t> m_chanmap;
    InputStream m_stream;
    SampleFormat m_format;
public:
    explicit AFSource(InputStream &stream);
    /*
     * XXX: AudioFile_GetSizeProc is called inside of AudioFileClose().
     * Therefore, we must first call AudioFileClose() before destruction.
     */
    ~AFSource() { m_af.attach(0, false); }
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const
    {
	return m_chanmap.size() ? &m_chanmap: 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count) { m_offset += count; }
private:
    void init();
};

#endif
