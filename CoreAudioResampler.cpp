#include "CoreAudioResampler.h"

CoreAudioResampler::CoreAudioResampler(const std::shared_ptr<ISource> src,
				       int rate,
				       uint32_t quality,
				       uint32_t complexity)
    : FilterBase(src),
      m_quality(quality),
      m_complexity(complexity),
      m_rate(rate),
      m_position(0),
      m_source(src)
{
    const AudioStreamBasicDescription &asbd = src->getSampleFormat();
    m_asbd = cautil::buildASBDForPCM(rate, asbd.mChannelsPerFrame,
				     32, kAudioFormatFlagIsFloat);
    /*
     * XXX:
     * Might be 1 sample off.
     * It seems that resultant length is inconsistent between line/norm/bats.
     */
    m_length = src->length();
    if (m_length != ~0ULL)
	m_length = m_length * m_rate / asbd.mSampleRate + .5;
    init();
}

void CoreAudioResampler::init()
{
    const AudioStreamBasicDescription &asbd = source()->getSampleFormat();
    m_converter = AudioConverterX(asbd, m_asbd);
    m_converter.setSampleRateConverterQuality(m_quality);
    m_converter.setSampleRateConverterComplexity(m_complexity);
    m_encoder.reset(new CoreAudioEncoder(m_converter));

    struct Dispose { static void call(ISink *x) {} };
    std::shared_ptr<ISink> sink(this, Dispose::call);
    m_encoder->setSource(m_source);
    m_encoder->setSink(sink);
}

size_t CoreAudioResampler::readSamples(void *buffer, size_t nsamples)
{
    m_buffer = buffer;
    uint32_t count = m_encoder->encodeChunk(nsamples);
    m_position += count;
    return count;
}

void CoreAudioResampler::writeSamples(const void *data, size_t len,
				      size_t nsamples)
{
    std::memcpy(m_buffer, data, len);
}
