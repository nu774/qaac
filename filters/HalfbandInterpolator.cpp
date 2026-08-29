#include "HalfbandInterpolator.h"
#include <cstring>
#include <stdexcept>
#include "KaiserLpf.h"
#include "ascutil.h"

namespace {
const double kTransitionWidth = 0.05; /* Fp..Fs width, as a fraction of Fc */
const double kSafety = 0.001;         /* keeps Fs strictly below Fc */
const double kAttenuationDb = 145.0;
const size_t kChunkFrames = 4096;
}

HalfbandInterpolator::HalfbandInterpolator(const std::shared_ptr<ISource> &src)
    : FilterBase(src), m_eof(false), m_position(0)
{
    const ca::AudioStreamBasicDescription &fmt = src->getSampleFormat();
    m_nchannels = fmt.mChannelsPerFrame;
    double Fin = fmt.mSampleRate;
    if (!(Fin > 0.0))
        throw std::runtime_error("HalfbandInterpolator: invalid sample rate");

    m_asbd = ascutil::buildASBDForPCM(Fin * 2.0, m_nchannels, 32,
                                     ca::kAudioFormatFlagIsFloat);

    double Fc = Fin / 2.0;
    double Fs = Fc * (1.0 - kSafety);
    double Fp = Fs - kTransitionWidth * Fc;
    std::vector<double> coefs = KaiserLpf::design(Fp, Fs, Fin, kAttenuationDb);
    for (size_t i = 0; i < coefs.size(); ++i)
        coefs[i] *= 2.0;   /* interpolation gain correction */

    m_zstuffed.set_unit(m_nchannels);
    m_filtered.set_unit(m_nchannels);
    for (unsigned i = 0; i < m_nchannels; ++i)
        m_convolvers.push_back(std::unique_ptr<StreamingConvolver>(
            new StreamingConvolver(coefs, coefs.size() >> 1)));

    m_scratch.assign(kChunkFrames * m_nchannels, 0.0f);

    m_length = source()->length();
    if (m_length != ~0ULL)
        m_length *= 2;
}

bool HalfbandInterpolator::fillFiltered(size_t minCount)
{
    while (m_filtered.count() < minCount) {
        if (!m_eof && m_zstuffed.count() == 0) {
            size_t n = readSamplesAsFloat(source(), &m_pivot, &m_scratch[0],
                                          kChunkFrames);
            if (n == 0) {
                m_eof = true;
            } else {
                m_zstuffed.reserve(n * 2);
                float *dst = m_zstuffed.write_ptr();
                for (size_t i = 0; i < n; ++i) {
                    for (unsigned ch = 0; ch < m_nchannels; ++ch) {
                        dst[i * 2 * m_nchannels + ch] =
                            m_scratch[i * m_nchannels + ch];
                        dst[(i * 2 + 1) * m_nchannels + ch] = 0.0f;
                    }
                }
                m_zstuffed.commit(n * 2);
            }
        }
        size_t ilen = m_zstuffed.count();
        size_t olen = minCount;
        m_filtered.reserve(olen);
        for (unsigned ch = 0; ch < m_nchannels; ++ch) {
            size_t ilen_ch = ilen, olen_ch = olen;
            m_convolvers[ch]->process(m_zstuffed.read_ptr() + ch,
                                      m_filtered.write_ptr() + ch,
                                      m_nchannels, m_nchannels,
                                      &ilen_ch, &olen_ch);
            olen = olen_ch;
        }
        m_zstuffed.advance(ilen);
        m_filtered.commit(olen);
        if (ilen == 0 && olen == 0)
            return false;
    }
    return true;
}

size_t HalfbandInterpolator::readSamples(void *buffer, size_t nsamples)
{
    float *op = static_cast<float *>(buffer);
    size_t produced = 0;
    while (produced < nsamples) {
        if (!fillFiltered(1))
            break;
        const float *ip = m_filtered.read_ptr();
        for (unsigned ch = 0; ch < m_nchannels; ++ch)
            op[produced * m_nchannels + ch] = ip[ch];
        m_filtered.advance(1);
        ++produced;
    }
    m_position += produced;
    return produced;
}
