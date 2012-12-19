#ifndef CoreAudioResampler_H
#define CoreAudioResampler_H

#include <deque>
#include "CoreAudioEncoder.h"

class CoreAudioResampler: public DelegatingSource, public ISink {
    AudioConverterX m_converter;
    x::shared_ptr<CoreAudioEncoder> m_encoder;
    std::deque<float> m_fbuffer;
    uint64_t m_samples_read;
    SampleFormat m_format;
public:
    CoreAudioResampler(const x::shared_ptr<ISource> src, int rate,
                       uint32_t quality, uint32_t complexity);
    uint64_t length() const { return -1; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    uint64_t getSamplesRead() const { return m_samples_read; }
    uint32_t getComplexity()
    {
        return m_converter.getSampleRateConverterComplexity();
    }
    uint32_t getQuality()
    {
        return m_converter.getSampleRateConverterQuality();
    }
    size_t readSamples(void *buffer, size_t nsamples);
    void writeSamples(const void *data, size_t len, size_t nsamples);
};
#endif
