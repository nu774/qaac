#ifndef INTEGER_SOURCE_H
#define INTEGER_SOURCE_H

#include <assert.h>
#include <random>
#include "iointer.h"

class IntegerSource: public DelegatingSource {
    SampleFormat m_format;
    std::mt19937 m_mt;
    std::uniform_real_distribution<double> m_dist;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float> m_fbuffer;
public:
    IntegerSource(const x::shared_ptr<ISource> &source, uint32_t bitdepth)
        : DelegatingSource(source), m_dist(-0.5, 0.5)
    {
        m_format = source->getSampleFormat();
        m_format.m_type = SampleFormat::kIsSignedInteger;
        m_format.m_bitsPerSample = bitdepth;
        m_format.m_endian = SampleFormat::kIsLittleEndian;
    }
    const SampleFormat &getSampleFormat() const { return m_format; }
    size_t readSamples(void *buffer, size_t nsamples);
private:
    double random() { return m_dist(m_mt); }
};

#endif
