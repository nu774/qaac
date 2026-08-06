#include "SoxLowpassFilter.h"
#include "KaiserLpf.h"
#include "cautil.h"
#include "ascutil.h"

SoxLowpassFilter::SoxLowpassFilter(const std::shared_ptr<ISource> &src,
                                   unsigned Fp)
    : FilterBase(src), m_position(0)
{
    const ca::AudioStreamBasicDescription &asbd = src->getSampleFormat();
    m_asbd = ascutil::buildASBDForPCM(asbd.mSampleRate, asbd.mChannelsPerFrame,
                                     32, kAudioFormatFlagIsFloat);
    m_buffer.set_unit(m_asbd.mChannelsPerFrame);

    double Fn = asbd.mSampleRate / 2.0;
    double Fs = Fp + asbd.mSampleRate * 0.0125;
    if (Fp == 0 || Fs > Fn)
        throw std::runtime_error("SoxLowpassFilter: invalid target rate");
    std::vector<double> coefs = KaiserLpf::design(Fp, Fs, Fn, 120.0);

    for (uint32_t i = 0; i < asbd.mChannelsPerFrame; ++i)
        m_convolvers.push_back(std::unique_ptr<StreamingConvolver>(
            new StreamingConvolver(coefs, coefs.size() >> 1)));
}

size_t SoxLowpassFilter::readSamples(void *buffer, size_t nsamples)
{
    uint32_t nchannels = m_asbd.mChannelsPerFrame;
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
        for (uint32_t ch = 0; ch < nchannels; ++ch) {
            size_t ilen_ch = ilen, olen_ch = nsamples;
            m_convolvers[ch]->process(m_buffer.read_ptr() + ch,
                                      static_cast<float *>(buffer) + ch,
                                      nchannels, nchannels,
                                      &ilen_ch, &olen_ch);
            olen = olen_ch;
        }
        m_buffer.advance(ilen);
    } while (ilen != 0 && olen == 0);

    m_position += olen;
    return olen;
}
