#ifndef AFSOURCE_H
#define AFSOURCE_H

#include "AudioFileX.h"
#include "ExtAudioFileX.h"
#include "iointer.h"
#include "win32util.h"

class ExtAFSource: public ISeekableSource, public ITagParser
{
    AudioFileX m_af;
    ExtAudioFileX m_eaf;
    uint64_t m_length;
    std::shared_ptr<FILE> m_fp;
    std::vector<uint32_t> m_chanmap;
    std::map<std::string, std::string> m_tags;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_iasbd, m_asbd;
public:
    ExtAFSource(const std::shared_ptr<FILE> &fp);
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
    int64_t getPosition();
    size_t readSamples(void *buffer, size_t nsamples);
    bool isSeekable() { return win32::is_seekable(fileno(m_fp.get())); }
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
};
#endif
