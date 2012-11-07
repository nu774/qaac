#ifndef AFSOURCE_H
#define AFSOURCE_H

#include "AudioFileX.h"
#include "ExtAudioFileX.h"
#include "iointer.h"

class ExtAFSource: public ITagParser, public PartialSource<ExtAFSource>
{
    AudioFileX m_af;
    ExtAudioFileX m_eaf;
    uint64_t m_offset;
    std::shared_ptr<FILE> m_fp;
    std::vector<uint32_t> m_chanmap;
    std::map<uint32_t, std::wstring> m_tags;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_asbd;
public:
    ExtAFSource(const std::shared_ptr<FILE> &fp);
    ~ExtAFSource()
    {
	m_af.attach(0, false);
	m_eaf.attach(0, false);
    }
    uint64_t length() const { return getDuration(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
	return m_chanmap.size() ? &m_chanmap: 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<chapters::entry_t> *getChapters() const { return 0; }
};
#endif
