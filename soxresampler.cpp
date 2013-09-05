#include "soxresampler.h"
#include "cautil.h"

SoxrResampler::SoxrResampler(const SOXRModule &module,
                             const std::shared_ptr<ISource> &src,
                             unsigned rate)
    : FilterBase(src), m_module(module), m_position(0)
{
    const AudioStreamBasicDescription &asbd = src->getSampleFormat();
    unsigned bits = 32;
    if (asbd.mBitsPerChannel > 32
        || (asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger) &&
           asbd.mBitsPerChannel > 24)
        bits = 64;
    m_asbd = cautil::buildASBDForPCM(rate, asbd.mChannelsPerFrame,
                                     bits, kAudioFormatFlagIsFloat);

    soxr_quality_spec_t qspec;
    soxr_io_spec_t iospec;
    if (bits == 32) {
        qspec = m_module.quality_spec(SOXR_HQ, 0);
        iospec = m_module.io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
    } else {
        qspec = m_module.quality_spec(SOXR_VHQ, 0);
        iospec = m_module.io_spec(SOXR_FLOAT64_I, SOXR_FLOAT64_I);
    }
    soxr_error_t error = 0;
    soxr_t resampler = m_module.create(asbd.mSampleRate, rate,
                                       asbd.mChannelsPerFrame,
                                       &error, &iospec, &qspec, 0);
    if (!resampler)
        throw std::runtime_error(strutil::format("soxr: %s",
                                                 soxr_strerror(error)));
    m_resampler = std::shared_ptr<soxr>(resampler, m_module.delete_);
    m_module.set_input_fn(resampler, staticInputProc, this, 0x10000);
    double factor = rate / asbd.mSampleRate;
    m_length = source()->length();
    if (m_length != ~0ULL)
        m_length = m_length * factor + .5;
}

size_t SoxrResampler::readSamples(void *buffer, size_t nsamples)
{
    size_t odone = m_module.output(m_resampler.get(), buffer, nsamples);
    m_position += odone;
    return odone;
}

size_t SoxrResampler::inputProc(soxr_in_t *data, size_t nsamples)
{
    if (m_buffer.size() < nsamples * m_asbd.mBytesPerFrame)
        m_buffer.resize(nsamples * m_asbd.mBytesPerFrame);
    if (m_asbd.mBitsPerChannel == 32)
        nsamples = readSamplesAsFloat(source(), &m_pivot,
                                      reinterpret_cast<float*>(&m_buffer[0]),
                                      nsamples);
    else
        nsamples = readSamplesAsFloat(source(), &m_pivot,
                                      reinterpret_cast<double*>(&m_buffer[0]),
                                      nsamples);
    *data = &m_buffer[0];
    return nsamples;
}

