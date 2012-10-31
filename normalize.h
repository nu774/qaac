#ifndef _NORMALIZE_H
#define _NORMALIZE_H

#include "iointer.h"

class Normalizer: public DelegatingSource {
    double m_peak;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float> m_fbuffer;
    x::shared_ptr<FILE> m_tmpfile;
    AudioStreamBasicDescription m_format;
    uint64_t m_processed;
    uint64_t m_samples_read;
public:
    Normalizer(const x::shared_ptr<ISource> &src);
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_format;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    double getPeak() const { return m_peak; }
    size_t process(size_t nsamples);
    uint64_t samplesRead() const { return m_processed; }
    uint64_t getSamplesRead() const { return m_samples_read; }
    uint64_t length() const { return m_processed; }
};

#endif
