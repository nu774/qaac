#ifndef _RAWSOURCE_H
#define _RAWSOURCE_H

#include "iointer.h"

class RawSource: public PartialSource<RawSource> {
    std::shared_ptr<FILE> m_fp;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_asbd, m_oasbd;
public:
    RawSource(const std::shared_ptr<FILE> &fp,
	      const AudioStreamBasicDescription &asbd);
    uint64_t length() const { return getDuration(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_oasbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
};

#endif
