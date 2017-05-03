#include "SoxLowpassFilter.h"
#include "cautil.h"

SoxLowpassFilter::SoxLowpassFilter(const SoXConvolverModule &module,
                                   const std::shared_ptr<ISource> &src,
                                   unsigned Fp)
    : FilterBase(src), m_position(0), m_module(module)
{
    const AudioStreamBasicDescription &asbd = src->getSampleFormat();
    m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate, asbd.mChannelsPerFrame,
                                     32, kAudioFormatFlagIsFloat);
    m_buffer.set_unit(m_asbd.mChannelsPerFrame);

    double Fn = asbd.mSampleRate / 2.0;
    double Fs = Fp + asbd.mSampleRate * 0.0125;
    if (Fp == 0 || Fs > Fn)
        throw std::runtime_error("SoxLowpassFilter: invalid target rate");
    int num_taps = 0;
    double *coefs = m_module.design_lpf(Fp, Fs, Fn, 120.0, &num_taps, 0, -1);
    if (!coefs)
        throw std::runtime_error("lsx_design_lpf()");
    std::shared_ptr<double> __delete_lator__(coefs, m_module.free);

    lsx_convolver_t *convolver =
        m_module.create(asbd.mChannelsPerFrame, coefs, num_taps, num_taps >> 1);
    if (!convolver)
        throw std::runtime_error("lsx_convolver_create()");
    m_convolver = std::shared_ptr<lsx_convolver_t>(convolver, m_module.close);
}

size_t SoxLowpassFilter::readSamples(void *buffer, size_t nsamples)
{
    size_t ilen = 0, olen = 0;
    do {
        if (m_buffer.count() == 0) {
            m_buffer.reserve(nsamples);
            size_t n = readSamplesAsFloat(source(), &m_pivot,
                                          m_buffer.write_ptr(), nsamples);
            m_buffer.commit(n);
        }
        ilen = m_buffer.count();
        olen = nsamples;
        m_module.process(m_convolver.get(), m_buffer.read_ptr(),
                         static_cast<float *>(buffer), &ilen, &olen);
        m_buffer.advance(ilen);
    } while (ilen != 0 && olen == 0);

    m_position += olen;
    return olen;
}
