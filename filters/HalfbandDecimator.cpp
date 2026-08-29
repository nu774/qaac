#include "HalfbandDecimator.h"
#include <cmath>
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

HalfbandDecimator::HalfbandDecimator(const std::shared_ptr<ISource> &src)
    : FilterBase(src), m_position(0)
{
    const ca::AudioStreamBasicDescription &fmt = src->getSampleFormat();
    m_nchannels = fmt.mChannelsPerFrame;
    double Fin = fmt.mSampleRate;
    if (!(Fin > 0.0) || std::fmod(Fin, 2.0) != 0.0)
        throw std::runtime_error(
            "HalfbandDecimator: sample rate must be an even integer");

    m_asbd = ascutil::buildASBDForPCM(Fin / 2.0, m_nchannels, 32,
                                     ca::kAudioFormatFlagIsFloat);

    double Fc = Fin / 4.0;
    double Fs = Fc * (1.0 - kSafety);
    double Fp = Fs - kTransitionWidth * Fc;
    std::vector<double> coefs = KaiserLpf::design(Fp, Fs, Fin / 2.0,
                                                  kAttenuationDb);

    m_raw.set_unit(m_nchannels);
    m_filtered.set_unit(m_nchannels);
    for (unsigned i = 0; i < m_nchannels; ++i)
        m_convolvers.push_back(std::unique_ptr<StreamingConvolver>(
            new StreamingConvolver(coefs, coefs.size() >> 1)));

    m_length = source()->length();
    if (m_length != ~0ULL)
        m_length /= 2;
}

bool HalfbandDecimator::fillFiltered(size_t minCount)
{
    while (m_filtered.count() < minCount) {
        if (m_raw.count() == 0) {
            m_raw.reserve(kChunkFrames);
            size_t n = readSamplesAsFloat(source(), &m_pivot,
                                          m_raw.write_ptr(), kChunkFrames);
            m_raw.commit(n);
        }
        size_t ilen = m_raw.count();
        size_t olen = minCount;
        m_filtered.reserve(olen);
        for (unsigned ch = 0; ch < m_nchannels; ++ch) {
            size_t ilen_ch = ilen, olen_ch = olen;
            m_convolvers[ch]->process(m_raw.read_ptr() + ch,
                                      m_filtered.write_ptr() + ch,
                                      m_nchannels, m_nchannels,
                                      &ilen_ch, &olen_ch);
            olen = olen_ch;
        }
        m_raw.advance(ilen);
        m_filtered.commit(olen);
        if (ilen == 0 && olen == 0)
            return false;
    }
    return true;
}

size_t HalfbandDecimator::readSamples(void *buffer, size_t nsamples)
{
    float *op = static_cast<float *>(buffer);
    size_t produced = 0;
    while (produced < nsamples) {
        if (!fillFiltered(2)) {
            if (m_filtered.count() == 1)
                m_filtered.advance(1); /* odd leftover at EOF: discard */
            break;
        }
        const float *ip = m_filtered.read_ptr();
        for (unsigned ch = 0; ch < m_nchannels; ++ch)
            op[produced * m_nchannels + ch] = ip[ch];
        m_filtered.advance(2); /* keep the 1st, drop the 2nd */
        ++produced;
    }
    m_position += produced;
    return produced;
}
