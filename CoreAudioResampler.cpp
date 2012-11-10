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
    uint64_t len = src->length();
    /*
     * XXX:
     * Might be 1 sample off.
     * It seems that resultant length is inconsistent between line/norm/bats.
     */
    m_length = (len == ~0ULL ? ~0ULL : len * m_rate / asbd.mSampleRate + .5);
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
    float *dst = static_cast<float*>(buffer);
    if (!m_fbuffer.size() && !m_encoder->encodeChunk(4096))
	return 0;
    size_t count = 0;
    if (m_fbuffer.size()) {
	for (size_t i = 0; i < m_asbd.mChannelsPerFrame; ++i) {
	    *dst++ = m_fbuffer.front();
	    m_fbuffer.pop_front();
	}
	++count;
    }
    m_position += count;
    return count;
}

void CoreAudioResampler::writeSamples(const void *data, size_t len,
				      size_t nsamples)
{
    const float *fp = static_cast<const float*>(data);
    for (size_t i = 0; i < len / sizeof(float); ++i)
	m_fbuffer.push_back(fp[i]);
}
