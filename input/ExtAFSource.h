#ifndef AFSOURCE_H
#define AFSOURCE_H

#include "AudioFileX.h"
#include "ExtAudioFileX.h"
#include "ISource.h"
#include "IInputStream.h"

class ExtAFSource: public ISeekableSource, public ITagParser
{
    AudioFileX m_af;
    ExtAudioFileX m_eaf;
    int64_t m_position;
    uint64_t m_length;
    std::shared_ptr<IInputStream> m_stream;
    std::vector<uint32_t> m_chanmap;
    std::map<std::string, std::string> m_tags;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_iasbd, m_asbd;
public:
    ExtAFSource(std::shared_ptr<IInputStream> stream);
    ~ExtAFSource()
    {
        m_af.attach(0, false);
        m_eaf.attach(0, false);
    }
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
        return m_chanmap.size() ? &m_chanmap: 0;
    }
    int64_t getPosition() { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples);
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }

    OSStatus readCallback(SInt64 pos, UInt32 count, void *data, UInt32 *nread);
    SInt64 sizeCallback();
    static OSStatus staticReadCallback(void *cookie, SInt64 pos, UInt32 count,
                                       void *data, UInt32 *nread) {
        return static_cast<ExtAFSource*>(cookie)->readCallback(pos, count, data, nread);
    }
    static SInt64 staticSizeCallback(void *cookie) {
        return static_cast<ExtAFSource*>(cookie)->sizeCallback();
    }
};
#endif
