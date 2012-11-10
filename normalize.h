#ifndef _NORMALIZE_H
#define _NORMALIZE_H

#include "iointer.h"

class Normalizer: public FilterBase {
    double m_peak;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float> m_fbuffer;
    std::shared_ptr<FILE> m_tmpfile;
    uint64_t m_processed, m_position;
    AudioStreamBasicDescription m_asbd;
public:
    Normalizer(const std::shared_ptr<ISource> &src, bool seekable);
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    double getPeak() const { return m_peak; }
    size_t process(size_t nsamples);
    int64_t getPosition() { return m_position; }
    uint64_t length() const { return m_processed; }
};

#endif
