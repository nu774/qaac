#ifndef CoreAudioResampler_H
#define CoreAudioResampler_H

#include <deque>
#include "CoreAudioEncoder.h"

class CoreAudioResampler: public FilterBase, public ISink {
    uint32_t m_quality, m_complexity;
    double m_rate;
    int64_t m_position;
    int64_t m_length;
    AudioConverterX m_converter;
    std::shared_ptr<CoreAudioEncoder> m_encoder;
    std::shared_ptr<ISource> m_source;
    std::deque<float> m_fbuffer;
    AudioStreamBasicDescription m_asbd;
public:
    CoreAudioResampler(const std::shared_ptr<ISource> src, int rate,
		       uint32_t quality, uint32_t complexity);
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    int64_t getPosition() { return m_position; }
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
private:
    void init();
};
#endif
