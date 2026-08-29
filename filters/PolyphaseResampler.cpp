#include "PolyphaseResampler.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include "KaiserLpf.h"
#include "ascutil.h"

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#define POLYPHASE_USE_SSE2 1
#include <emmintrin.h>
#endif

namespace {
const size_t kPhases = 256;
const double kAttenuationDb = 145.0;
const double kTransitionWidth = 0.05; /* Fp..Fs width, as a fraction of Fc */
const double kSafety = 0.001;         /* keeps Fs strictly below Fc */
const size_t kChunkFrames = 4096;

#ifdef POLYPHASE_USE_SSE2
inline float hsum128(__m128 v)
{
    __m128 shuf = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 0, 3, 2));
    __m128 sums = _mm_add_ps(v, shuf);
    shuf = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(2, 3, 0, 1));
    sums = _mm_add_ps(sums, shuf);
    float result;
    _mm_store_ss(&result, sums);
    return result;
}

inline void dotProductPair(const float *tap0, const float *tap1,
                           const float *ipc0, const float *ipc1,
                           size_t n, float *sum0, float *sum1)
{
    __m128 vacc0 = _mm_setzero_ps();
    __m128 vacc1 = _mm_setzero_ps();
    size_t j = 0;
    size_t limit = n - (n % 4);
    for (; j < limit; j += 4) {
        vacc0 = _mm_add_ps(vacc0,
            _mm_mul_ps(_mm_loadu_ps(tap0 + j), _mm_loadu_ps(ipc0 + j)));
        vacc1 = _mm_add_ps(vacc1,
            _mm_mul_ps(_mm_loadu_ps(tap1 + j), _mm_loadu_ps(ipc1 + j)));
    }
    float s0 = hsum128(vacc0);
    float s1 = hsum128(vacc1);
    for (; j < n; ++j) {
        s0 += tap0[j] * ipc0[j];
        s1 += tap1[j] * ipc1[j];
    }
    *sum0 = s0;
    *sum1 = s1;
}
#else
inline void dotProductPair(const float *tap0, const float *tap1,
                           const float *ipc0, const float *ipc1,
                           size_t n, float *sum0, float *sum1)
{
    float acc0[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float acc1[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    size_t j = 0;
    size_t limit = n - (n % 4);
    for (; j < limit; j += 4) {
        for (size_t lane = 0; lane < 4; ++lane) {
            acc0[lane] += tap0[j + lane] * ipc0[j + lane];
            acc1[lane] += tap1[j + lane] * ipc1[j + lane];
        }
    }
    float s0 = (acc0[0] + acc0[1]) + (acc0[2] + acc0[3]);
    float s1 = (acc1[0] + acc1[1]) + (acc1[2] + acc1[3]);
    for (; j < n; ++j) {
        s0 += tap0[j] * ipc0[j];
        s1 += tap1[j] * ipc1[j];
    }
    *sum0 = s0;
    *sum1 = s1;
}
#endif
}

PolyphaseResampler::PolyphaseResampler(const std::shared_ptr<ISource> &src,
                                       unsigned rate)
    : FilterBase(src), m_phases(kPhases),
      m_realFramesIn(0), m_tailPadded(false), m_position(0)
{
    const ca::AudioStreamBasicDescription &fmt = src->getSampleFormat();
    m_nchannels = fmt.mChannelsPerFrame;
    double Fin = fmt.mSampleRate;
    double Fout = rate;
    if (!(Fin > 0.0) || !(Fout > 0.0))
        throw std::runtime_error("PolyphaseResampler: invalid sample rate");

    m_asbd = ascutil::buildASBDForPCM(Fout, m_nchannels, 32,
                                     ca::kAudioFormatFlagIsFloat);

    double Fc = (std::min)(Fin, Fout) / 2.0;
    double Fs = Fc * (1.0 - kSafety);
    double Fp = Fs - kTransitionWidth * Fc;
    double Fn = Fin * m_phases / 2.0;
    std::vector<double> coefs = KaiserLpf::design(Fp, Fs, Fn, kAttenuationDb);
    for (size_t i = 0; i < coefs.size(); ++i)
        coefs[i] *= static_cast<double>(m_phases);

    size_t numTaps = coefs.size();
    m_tapsPerPhase = (numTaps + m_phases - 1) / m_phases;

    int64_t center = std::llround((static_cast<double>(numTaps) - 1.0) / 2.0);
    int64_t centerRows = center / static_cast<int64_t>(m_phases);
    int64_t centerPhase = center % static_cast<int64_t>(m_phases);
    m_windowBase = centerRows - static_cast<int64_t>(m_tapsPerPhase) + 1;

    coefs.resize(m_tapsPerPhase * m_phases + centerPhase, 0.0);

    m_table.assign(m_phases * m_tapsPerPhase, 0.0f);
    for (size_t k = 0; k < m_phases; ++k) {
        for (size_t j = 0; j < m_tapsPerPhase; ++j) {
            size_t idx = (m_tapsPerPhase - 1 - j) * m_phases + k
                       + static_cast<size_t>(centerPhase);
            m_table[k * m_tapsPerPhase + j] = static_cast<float>(coefs[idx]);
        }
    }

    m_step = Fin / Fout;
    m_scratch.assign(kChunkFrames * m_nchannels, 0.0f);

    m_buffer.resize(m_nchannels);
    m_bufBaseFrame = m_windowBase;
    if (m_windowBase < 0) {
        size_t n = static_cast<size_t>(-m_windowBase);
        for (unsigned ch = 0; ch < m_nchannels; ++ch) {
            m_buffer[ch].reserve(n);
            std::memset(m_buffer[ch].write_ptr(), 0, n * sizeof(float));
            m_buffer[ch].commit(n);
        }
    }

    m_length = source()->length();
    if (m_length != ~0ULL) {
        double factor = Fout / Fin;
        m_length = static_cast<uint64_t>(m_length * factor + 0.5);
    }
}

bool PolyphaseResampler::fill(int64_t throughFrame)
{
    while (m_bufBaseFrame + static_cast<int64_t>(m_buffer[0].count())
           <= throughFrame)
    {
        if (m_tailPadded)
            return false;
        size_t n = readSamplesAsFloat(source(), &m_pivot, &m_scratch[0],
                                      kChunkFrames);
        if (n == 0) {
            for (unsigned ch = 0; ch < m_nchannels; ++ch) {
                m_buffer[ch].reserve(m_tapsPerPhase);
                std::memset(m_buffer[ch].write_ptr(), 0,
                           m_tapsPerPhase * sizeof(float));
                m_buffer[ch].commit(m_tapsPerPhase);
            }
            m_tailPadded = true;
        } else {
            for (unsigned ch = 0; ch < m_nchannels; ++ch) {
                m_buffer[ch].reserve(n);
                float *dst = m_buffer[ch].write_ptr();
                for (size_t i = 0; i < n; ++i)
                    dst[i] = m_scratch[i * m_nchannels + ch];
                m_buffer[ch].commit(n);
            }
            m_realFramesIn += n;
        }
    }
    return true;
}

size_t PolyphaseResampler::readSamples(void *buffer, size_t nsamples)
{
    float *op = static_cast<float *>(buffer);
    size_t produced = 0;
    while (produced < nsamples) {
        double pos = static_cast<double>(m_position + produced) * m_step;
        int64_t i = static_cast<int64_t>(std::floor(pos));
        double f = pos - static_cast<double>(i);

        double phasePos = f * m_phases;
        size_t k0 = static_cast<size_t>(phasePos);
        double frac = phasePos - static_cast<double>(k0);
        size_t k1 = k0 + 1;
        int64_t extraShift = 0;
        if (k1 == m_phases) { k1 = 0; extraShift = 1; }

        if (m_tailPadded && i >= m_realFramesIn)
            break;

        int64_t windowStart = i + m_windowBase;
        int64_t windowEnd = windowStart + extraShift
                           + static_cast<int64_t>(m_tapsPerPhase) - 1;
        if (!fill(windowEnd))
            break;

        int64_t base = windowStart - m_bufBaseFrame;
        const float *tap0 = &m_table[k0 * m_tapsPerPhase];
        const float *tap1 = &m_table[k1 * m_tapsPerPhase];
        for (unsigned ch = 0; ch < m_nchannels; ++ch) {
            const float *ipc0 = m_buffer[ch].read_ptr() + base;
            const float *ipc1 = ipc0 + extraShift;
            float sum0, sum1;
            dotProductPair(tap0, tap1, ipc0, ipc1, m_tapsPerPhase,
                          &sum0, &sum1);
            double acc = sum0 + frac * (sum1 - sum0);
            op[produced * m_nchannels + ch] = static_cast<float>(acc);
        }
        ++produced;

        double nextPos = static_cast<double>(m_position + produced) * m_step;
        int64_t newI = static_cast<int64_t>(std::floor(nextPos));
        int64_t releaseUpto = newI + m_windowBase;
        int64_t drop = releaseUpto - m_bufBaseFrame;
        if (drop > 0) {
            size_t avail = m_buffer[0].count();
            if (static_cast<size_t>(drop) > avail)
                drop = static_cast<int64_t>(avail);
            for (unsigned ch = 0; ch < m_nchannels; ++ch)
                m_buffer[ch].advance(static_cast<size_t>(drop));
            m_bufBaseFrame += drop;
        }
    }
    m_position += produced;
    return produced;
}
