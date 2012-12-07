#ifndef _NULLSOURCE_H
#define _NULLSOURCE_H

#include "iointer.h"

class NullSource: public ISeekableSource {
    AudioStreamBasicDescription m_asbd;
    int64_t m_position;
public:
    NullSource(const AudioStreamBasicDescription &asbd):
        m_asbd(asbd), m_position(0)
    {}
    uint64_t length() const { return -1; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
        std::memset(buffer, 0, nsamples * m_asbd.mBytesPerFrame);
        m_position += nsamples;
        return nsamples;
    }
    bool isSeekable() { return true; }
    void seekTo(int64_t count) { m_position = count; }
    int64_t getPosition() { return m_position; }
};

#endif
