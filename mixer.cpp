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
        float sum = 0.0;
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
                         const SoXConvolverModule &module,
                         const std::vector<std::vector<complex_t> > &spec,
                         bool normalize)
    : FilterBase(source),
      m_position(0),
      m_matrix(spec),
      m_module(module)
{
    const AudioStreamBasicDescription &fmt = source->getSampleFormat();
    uint32_t shiftMask;
    if (!validateMatrix(m_matrix, &shiftMask))
        throw std::runtime_error("invalid/unsupported matrix spec");
    if (m_matrix[0].size() != fmt.mChannelsPerFrame)
        throw std::runtime_error("unmatch number of channels with matrix");
    if (normalize)
        normalizeMatrix(m_matrix);
    m_asbd = cautil::buildASBDForPCM(fmt.mSampleRate, spec.size(),
                                     32, kAudioFormatFlagIsFloat);
    m_buffer.set_unit(fmt.mChannelsPerFrame);
    for (unsigned i = 0; i < fmt.mChannelsPerFrame; ++i) {
        if (shiftMask & (1 << i))
            m_shift_channels.push_back(i);
        else
            m_pass_channels.push_back(i);
    }
    if (shiftMask)
        initFilter();
}

void MatrixMixer::initFilter()
{
    const AudioStreamBasicDescription &fmt = source()->getSampleFormat();
    size_t numtaps = fmt.mSampleRate / 12;
    if (!(numtaps & 1)) ++numtaps;
    std::vector<double> coefs(numtaps);
    hilbert(&coefs[0], numtaps);
    applyHamming(&coefs[0], numtaps);
    double filter_gain = calcGain(&coefs[0], numtaps);
    for (std::vector<double>::iterator ii = coefs.begin();
         ii != coefs.end(); ++ii)
        *ii /= filter_gain;

    for (unsigned i = 0; i < m_shift_channels.size(); ++i) {
        lsx_convolver_t *f =
            m_module.create(1, &coefs[0], coefs.size(), coefs.size()>>1);
        if (!f)
            throw std::runtime_error("failed to init hilbert transformer");
        std::shared_ptr<lsx_convolver_t> filterp(f, m_module.close);
        m_filter.push_back(filterp);
    }
}

size_t MatrixMixer::readSamples(void *buffer, size_t nsamples)
{
    uint32_t ichannels = source()->getSampleFormat().mChannelsPerFrame;
    if (m_fbuffer.size() < nsamples * ichannels)
        m_fbuffer.resize(nsamples * ichannels);

    if (m_shift_channels.size())
        nsamples = phaseShift(nsamples);
    else
        nsamples = readSamplesAsFloat(source(), &m_ibuffer,
                                      &m_fbuffer[0], nsamples);

    float *op = static_cast<float*>(buffer);
    for (size_t i = 0; i < nsamples; ++i) {
        float *ip = &m_fbuffer[i * ichannels];
        for (size_t out = 0; out < m_asbd.mChannelsPerFrame; ++out) {
            float value = 0.0f;
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
    const uint32_t ichannels = source()->getSampleFormat().mChannelsPerFrame;
    const size_t pass_channels_size = m_pass_channels.size();
    const size_t shift_channels_size = m_shift_channels.size();
    const unsigned * const pass_channels =
        pass_channels_size ? &m_pass_channels[0]: 0;
    const unsigned * const shift_channels = &m_shift_channels[0];

    size_t ilen = 0, olen = 0;
    do {
        if (m_buffer.count() == 0) {
            m_buffer.reserve(nsamples);
            ilen = readSamplesAsFloat(source(), &m_ibuffer,
                                      m_buffer.write_ptr(), nsamples);
            m_buffer.commit(ilen);
            if (pass_channels_size > 0) {
                for (size_t i = 0; i < ilen; ++i) {
                    float *frame = m_buffer.read_ptr() + i * ichannels;
                    for (unsigned n = 0; n < pass_channels_size; ++n)
                        m_syncque.push_back(frame[pass_channels[n]]);
                }
            }
        }
        float *bp = m_buffer.read_ptr();
        for (unsigned i = 0; i < shift_channels_size; ++i) {
            unsigned n = shift_channels[i];
            ilen = m_buffer.count();
            olen = nsamples;
            float *ip = bp + n;
            float *op = &m_fbuffer[n];
            m_module.process_ni(m_filter[i].get(), &ip, &op,
                                ichannels, ichannels, &ilen, &olen);
        }
        m_buffer.advance(ilen);
    } while (ilen != 0 && olen == 0);

    if (pass_channels_size > 0) {
        float *fp = &m_fbuffer[0];
        for (size_t i = 0; i < olen; ++i) {
            float *frame = fp + i * ichannels;
            for (unsigned n = 0; n < pass_channels_size; ++n) {
                frame[pass_channels[n]] = m_syncque.front();
                m_syncque.pop_front();
            }
        }
    }
    return olen;
}
