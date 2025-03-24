#ifndef _RAWSOURCE_H
#define _RAWSOURCE_H

#include "ISource.h"
#include "win32util.h"
#include "IInputStream.h"

class RawSource: public ISeekableSource {
    uint64_t m_length;
    int64_t m_position;
    std::shared_ptr<IInputStream> m_stream;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_asbd, m_oasbd;
public:
    RawSource(std::shared_ptr<IInputStream> stream,
              const AudioStreamBasicDescription &asbd);
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_oasbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples);
    void seekTo(int64_t count);
    int64_t getPosition() { return m_position; }
};

#endif
