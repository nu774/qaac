#include "StreamingConvolver.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include "fft4g_float.h"

namespace {

size_t nextPow2(size_t n)
{
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

/*
 * fft4g's rdft() needs a bit-reversal work area of this size (documented
 * in fft4g.c itself); this is a property of the FFT algorithm, not a
 * design choice of ours.
 */
size_t fftWorkIntLength(size_t n)
{
    int bits = static_cast<int>(std::log(n / 2.0 + 0.5) / std::log(2.0));
    return 2 + (static_cast<size_t>(1) << (bits / 2));
}

} // namespace

StreamingConvolver::StreamingConvolver(const std::vector<double> &coefs,
                                       size_t postPeak)
    : m_numTaps(coefs.size()), m_realSamplesIn(0), m_totalDelivered(0),
      m_finished(false)
{
    if (m_numTaps == 0)
        throw std::runtime_error("StreamingConvolver: empty filter");

    m_dftLength = nextPow2((std::max)(m_numTaps * 4, static_cast<size_t>(64)));
    m_blockAdvance = m_dftLength - (m_numTaps - 1);

    m_fftIp.assign(fftWorkIntLength(m_dftLength), 0);
    m_fftW.resize(m_dftLength / 2);
    m_scratch.resize(m_dftLength);

    /*
     * Precompute the (zero-padded) filter's spectrum. fft4g's inverse
     * rdft() is unnormalized: a forward/inverse round trip scales the
     * signal by dftLength/2, so we fold the compensating 2/dftLength
     * factor into the coefficient spectrum once, here, rather than
     * rescaling every block later.
     */
    m_coefsFreq.assign(m_dftLength, 0.0f);
    for (size_t i = 0; i < m_numTaps; ++i)
        m_coefsFreq[i] = static_cast<float>(coefs[i]);
    rdft(static_cast<int>(m_dftLength), 1, m_coefsFreq.data(),
        m_fftIp.data(), m_fftW.data());
    float scale = 2.0f / static_cast<float>(m_dftLength);
    for (size_t i = 0; i < m_dftLength; ++i)
        m_coefsFreq[i] *= scale;

    if (postPeak > 0) {
        m_pending.reserve(postPeak);
        std::memset(m_pending.write_ptr(), 0, postPeak * sizeof(float));
        m_pending.commit(postPeak);
    }
}

void StreamingConvolver::feed(const float *ibuf, size_t n, size_t stride)
{
    m_pending.reserve(n);
    float *dst = m_pending.write_ptr();
    if (stride == 1) {
        std::memcpy(dst, ibuf, n * sizeof(float));
    } else {
        for (size_t i = 0; i < n; ++i)
            dst[i] = ibuf[i * stride];
    }
    m_pending.commit(n);
}

void StreamingConvolver::convolveBlock(float *block)
{
    rdft(static_cast<int>(m_dftLength), 1, block, m_fftIp.data(),
        m_fftW.data());
    /* complex multiply against the filter's spectrum, in fft4g's
     * half-complex layout: block[0]/block[1] are the (real-valued) DC
     * and Nyquist bins, block[2k]/block[2k+1] are real/imag of bin k. */
    block[0] *= m_coefsFreq[0];
    block[1] *= m_coefsFreq[1];
    for (size_t i = 2; i < m_dftLength; i += 2) {
        float ar = block[i], ai = block[i + 1];
        float br = m_coefsFreq[i], bi = m_coefsFreq[i + 1];
        block[i]     = ar * br - ai * bi;
        block[i + 1] = ar * bi + ai * br;
    }
    rdft(static_cast<int>(m_dftLength), -1, block, m_fftIp.data(),
        m_fftW.data());
}

void StreamingConvolver::runBlocks(size_t maxBuffered)
{
    while (m_pending.count() >= m_dftLength && m_output.count() < maxBuffered) {
        std::memcpy(m_scratch.data(), m_pending.read_ptr(),
                   m_dftLength * sizeof(float));
        convolveBlock(m_scratch.data());

        size_t produced = m_blockAdvance;
        size_t room = maxBuffered - m_output.count();
        if (produced > room)
            produced = room;

        m_output.reserve(produced);
        std::memcpy(m_output.write_ptr(), m_scratch.data() + (m_numTaps - 1),
                   produced * sizeof(float));
        m_output.commit(produced);

        m_pending.advance(m_blockAdvance);
    }
}

void StreamingConvolver::process(const float *ibuf, float *obuf,
                                 size_t istride, size_t ostride,
                                 size_t *ilen, size_t *olen)
{
    size_t nin = *ilen;
    if (nin > 0) {
        feed(ibuf, nin, istride);
        m_realSamplesIn += nin;
        runBlocks((std::numeric_limits<size_t>::max)());
    } else if (!m_finished) {
        m_finished = true;
        size_t owed = static_cast<size_t>(m_realSamplesIn - m_totalDelivered);
        std::vector<float> zero(owed + m_dftLength, 0.0f);
        feed(zero.data(), zero.size(), 1);
        runBlocks(owed);
    }

    size_t want = *olen;
    size_t have = m_output.count();
    size_t n = (std::min)(want, have);
    if (n > 0) {
        const float *src = m_output.read_ptr();
        if (ostride == 1) {
            std::memcpy(obuf, src, n * sizeof(float));
        } else {
            for (size_t i = 0; i < n; ++i)
                obuf[i * ostride] = src[i];
        }
        m_output.advance(n);
    }
    m_totalDelivered += n;
    *olen = n;
}
