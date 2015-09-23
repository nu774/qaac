#ifndef _RAWSOURCE_H
#define _RAWSOURCE_H

#include "iointer.h"
#include "win32util.h"

class RawSource: public ISeekableSource {
    uint64_t m_length;
    int64_t m_position;
    std::shared_ptr<FILE> m_fp;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_asbd, m_oasbd;
public:
    RawSource(const std::shared_ptr<FILE> &fp,
              const AudioStreamBasicDescription &asbd);
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_oasbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples);
    bool isSeekable() { return win32::is_seekable(fileno(m_fp.get())); }
    void seekTo(int64_t count);
    int64_t getPosition() { return m_position; }
};

#endif
