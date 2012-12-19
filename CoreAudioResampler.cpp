#include "CoreAudioResampler.h"

CoreAudioResampler::CoreAudioResampler(const x::shared_ptr<ISource> src,
                                       int rate,
                                       uint32_t quality,
                                       uint32_t complexity)
    : DelegatingSource(src),
      m_samples_read(0)
{
    const SampleFormat &sfmt = src->getSampleFormat();
    m_format = SampleFormat("F32L", sfmt.m_nchannels, rate);

    AudioStreamBasicDescription iasbd, oasbd;
    BuildASBDFromSampleFormat(sfmt, &iasbd);
    BuildASBDFromSampleFormat(m_format, &oasbd);

    m_converter = AudioConverterX(iasbd, oasbd);
    m_converter.setSampleRateConverterQuality(quality);
    m_converter.setSampleRateConverterComplexity(complexity);
    m_encoder.reset(new CoreAudioEncoder(m_converter));

    struct F { static void dispose(ISink *x) {} };
    x::shared_ptr<ISink> sinkPtr(this, F::dispose);
    m_encoder->setSource(src);
    m_encoder->setSink(sinkPtr);
}

size_t CoreAudioResampler::readSamples(void *buffer, size_t nsamples)
{
    float *dst = static_cast<float*>(buffer);
    size_t processed = 0;
    while (processed < nsamples) {
        if (m_fbuffer.size() >= m_format.m_nchannels) {
            for (size_t i = 0; i < m_format.m_nchannels; ++i) {
                *dst++ = m_fbuffer.front();
                m_fbuffer.pop_front();
            }
            ++processed;
        } else if (!m_encoder->encodeChunk(8192))
            break;
    }
    m_samples_read += processed;
    return processed;
}

void CoreAudioResampler::writeSamples(const void *data, size_t len,
                                      size_t nsamples)
{
    const float *fp = static_cast<const float*>(data);
    for (size_t i = 0; i < len / sizeof(float); ++i)
        m_fbuffer.push_back(fp[i]);
}
