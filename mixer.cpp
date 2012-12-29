#include "mixer.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "cautil.h"

static bool validateMatrix(const std::vector<std::vector<complex_t> > &mat,
                           uint32_t *nshifts)
{
    *nshifts = 0;
    if (!mat.size()) return false;
    size_t size = mat[0].size();
    if (!size) return false;
    for (size_t out = 1; out < mat.size(); ++out)
        if (mat[out].size() != size)
            return false;
    for (size_t in = 0; in < size; ++in) {
        bool shift = mat[0][in].imag() != 0.0;
        if (shift && mat[0][in].real() != 0.0)
            return false;
        for (size_t out = 1; out < mat.size(); ++out)
            if (shift && mat[out][in].real() != 0.0)
                return false;
        if (shift) *nshifts |= (1<<in);
    }
    return true;
}

static void normalizeMatrix(std::vector<std::vector<complex_t> > &mat)
{
    for (size_t out = 0; out < mat.size(); ++out) {
        double sum = 0.0;
        for (size_t in = 0; in < mat[out].size(); ++in)
            sum += std::abs(mat[out][in]);
        if (!sum) continue;
        for (size_t in = 0; in < mat[out].size(); ++in)
            mat[out][in] /= sum;
    }
}

static void hilbert(double *coefs, size_t numcoefs)
{
    size_t i, origin = (numcoefs - 1) >> 1;
    coefs[origin] = 0.0;
    for (i = 1; i <= origin; ++i) {
        double x = (i & 1) ? 1.0 / i : 0.0;
        coefs[origin + i] = -x;
        coefs[origin - i] = x;
    }
}

static void applyHamming(double *coefs, size_t numcoefs)
{
    size_t i, origin = (numcoefs - 1) >> 1;
    for (i = 1; i <= origin; ++i) {
        double w = 0.54 - 0.46 *
            cos(2.0 * M_PI * (origin + i) / (numcoefs - 1));
        coefs[origin + i] *= w;
        coefs[origin - i] *= w;
    }
}

static double calcGain(double *coefs, size_t numcoefs)
{
    double gain = 0.0;
    size_t i, origin = (numcoefs - 1) >> 1;
    int odd = 0;
    for (i = origin + 1; i < numcoefs; i += 2) {
        if (odd) gain += coefs[i];
        else gain -= coefs[i];
        odd ^= 1;
    }
    if (odd) gain *= -1;
    gain *= 2.0;
    return gain;
}

MatrixMixer::MatrixMixer(const std::shared_ptr<ISource> &source,
                         const SoxModule &module,
                         const std::vector<std::vector<complex_t> > &spec,
                         bool mt,
                         bool normalize)
    : FilterBase(source),
      m_mt(mt),
      m_position(0),
      m_matrix(spec),
      m_module(module)
{
    const AudioStreamBasicDescription &fmt = source->getSampleFormat();
    if (!validateMatrix(m_matrix, &m_shiftMask))
        throw std::runtime_error("invalid/unsupported matrix spec");
    if (m_matrix[0].size() != fmt.mChannelsPerFrame)
        throw std::runtime_error("unmatch number of channels with matrix");
    if (normalize)
        normalizeMatrix(m_matrix);
    m_asbd = cautil::buildASBDForPCM(fmt.mSampleRate, spec.size(),
                                     64, kAudioFormatFlagIsFloat);
    m_buffer.units_per_packet = fmt.mChannelsPerFrame;
    if (m_shiftMask)
        initFilter();
}

void MatrixMixer::initFilter()
{
    const AudioStreamBasicDescription &fmt = source()->getSampleFormat();
    size_t numtaps = fmt.mSampleRate / 12;
    if (!(numtaps & 1)) ++numtaps;
    m_coefs.resize(numtaps);
    hilbert(&m_coefs[0], numtaps);
    applyHamming(&m_coefs[0], numtaps);
    m_filter_gain = calcGain(&m_coefs[0], numtaps);

    lsx_fir_t *filter =
        m_module.fir_create(util::bitcount(m_shiftMask), &m_coefs[0],
                            m_coefs.size(), m_coefs.size() / 2, m_mt);
    if (!filter)
        throw std::runtime_error("failed to init hilbert transformer");
    m_filter.reset(filter, m_module.fir_close);
    if (m_module.fir_start(m_filter.get()) < 0)
        throw std::runtime_error("failed to init hilbert transformer");
}

size_t MatrixMixer::readSamples(void *buffer, size_t nsamples)
{
    uint32_t ichannels = source()->getSampleFormat().mChannelsPerFrame;
    if (m_fbuffer.size() < nsamples * ichannels)
        m_fbuffer.resize(nsamples * ichannels);

    if (m_shiftMask)
        nsamples = phaseShift(nsamples);
    else
        nsamples = readSamplesAsFloat(source(), &m_ibuffer,
                                      &m_fbuffer[0], nsamples);

    double *op = static_cast<double*>(buffer);
    for (size_t i = 0; i < nsamples; ++i) {
        double *ip = &m_fbuffer[i * ichannels];
        for (size_t out = 0; out < m_asbd.mChannelsPerFrame; ++out) {
            double value = 0.0;
            for (size_t in = 0; in < ichannels; ++in) {
                complex_t factor = m_matrix[out][in];
                value += ip[in] * (factor.real() + factor.imag());
            }
            *op++ = value;
        }
    }
    m_position += nsamples;
    return nsamples;
}

size_t MatrixMixer::phaseShift(size_t nsamples)
{
    const int IOSIZE = 4096;
    uint32_t ichannels = source()->getSampleFormat().mChannelsPerFrame;
    uint32_t nshifts = util::bitcount(m_shiftMask);

    double **ivec = static_cast<double**>(_alloca(sizeof(double*) * nshifts));
    double **ovec = static_cast<double**>(_alloca(sizeof(double*) * nshifts));

    for (unsigned i = 0, n = 0; n < ichannels; ++n)
        if (m_shiftMask & (1 << n))
            ovec[i++] = &m_fbuffer[n];

    size_t ilen = 0, olen = 0;
    m_buffer.resize(IOSIZE);
    do {
        if (m_buffer.count() == 0) {
            ilen = readSamplesAsFloat(source(), &m_ibuffer,
                                      m_buffer.write_ptr(), IOSIZE);
            m_buffer.commit(ilen);
            for (size_t i = 0; i < ilen; ++i) {
                double *frame = m_buffer.read_ptr() + i * ichannels;
                for (unsigned n = 0; n < ichannels; ++n)
                    if (m_shiftMask & (1 << n))
                        frame[n] /= m_filter_gain;
                    else
                        m_syncque.push_back(frame[n]);
            }
        }
        double *bp = m_buffer.read_ptr();
        for (unsigned i = 0, n = 0; n < ichannels; ++n)
            if (m_shiftMask & (1 << n))
                ivec[i++] = bp + n;
        ilen = m_buffer.count();
        olen = nsamples;
        m_module.fir_process_d(m_filter.get(), ivec, ovec, &ilen, &olen,
                               ichannels, ichannels);
        m_buffer.advance(ilen);
    } while (ilen != 0 && olen == 0);

    for (size_t i = 0; i < olen; ++i) {
        double *frame = &m_fbuffer[i * ichannels];
        for (unsigned n = 0; n < ichannels; ++n) {
            if (!(m_shiftMask & (1 << n))) {
                frame[n] = m_syncque.front();
                m_syncque.pop_front();
            }
        }
    }
    return olen;
}
