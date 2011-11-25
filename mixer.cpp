#include "mixer.h"
#define _USE_MATH_DEFINES
#include <math.h>

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

MatrixMixer::MatrixMixer(const x::shared_ptr<ISource> &source,
			 const SoxModule &module,
			 const std::vector<std::vector<complex_t> > &spec)
    : DelegatingSource(source), m_module(module), m_matrix(spec),
      m_input_frames(0), m_end_of_input(false)
{
    const SampleFormat &fmt = source->getSampleFormat();
    if (!validateMatrix(m_matrix, &m_shiftMask))
	throw std::runtime_error("invalid/unsupported matrix spec");
    if (m_matrix[0].size() != fmt.m_nchannels)
	throw std::runtime_error("unmatch number of channels with matrix");
    normalizeMatrix(m_matrix);
    m_format = SampleFormat("F32LE", spec.size(), fmt.m_rate);

    if (m_shiftMask) {
	size_t numtaps = fmt.m_rate / 12;
	if (!(numtaps & 1)) ++numtaps;
	std::vector<double> coefs(numtaps);
	hilbert(&coefs[0], numtaps);
	applyHamming(&coefs[0], numtaps);
	m_filter_gain = calcGain(&coefs[0], numtaps);
	lsx_fir_t *filter =
	    m_module.fir_create(bitcount(m_shiftMask), &coefs[0], numtaps,
				numtaps >> 1, true);
	if (!filter)
	    throw std::runtime_error("failed to init hilbert transformer");
	m_filter = x::shared_ptr<lsx_fir_t>(filter, m_module.fir_close);
	if (m_module.fir_start(m_filter.get()) < 0)
	    throw std::runtime_error("failed to init hilbert transformer");

	for (size_t i = 0; i < fmt.m_nchannels; ++i)
	    if (m_shiftMask & (1 << i))
		m_shift_channels.push_back(i);
	    else
		m_pass_channels.push_back(i);
    }
}

size_t MatrixMixer::readSamples(void *buffer, size_t nsamples)
{
    uint32_t ichannels = source()->getSampleFormat().m_nchannels;
    if (m_fbuffer[0].size() < nsamples * ichannels)
	m_fbuffer[0].resize(nsamples * ichannels);
    if (!m_shiftMask)
	nsamples = readSamplesAsFloat(source(), &m_ibuffer, &m_fbuffer[0],
				      nsamples);
    else
	nsamples = phaseShift(&m_fbuffer[0][0], nsamples);

    float *ip = &m_fbuffer[0][0];
    float *op = static_cast<float*>(buffer);
    for (size_t i = 0; i < nsamples; ++i) {
	for (size_t out = 0; out < m_matrix.size(); ++out) {
	    double value = 0.0;
	    for (size_t in = 0; in < m_matrix[out].size(); ++in) {
		complex_t factor = m_matrix[out][in];
		float f = ip[i * ichannels + in];
		value += f * (factor.real() + factor.imag());
	    }
	    *op++ = value;
	}
    }
    return nsamples;
}

size_t MatrixMixer::phaseShift(void *buffer, size_t nsamples)
{
    float *src = m_fbuffer[1].size() ? &m_fbuffer[1][0] : 0;
    float *dst = static_cast<float*>(buffer);

    uint32_t ichannels = source()->getSampleFormat().m_nchannels;
    uint32_t nshifts = bitcount(m_shiftMask);
    std::vector<float*> ivec(nshifts), ovec(nshifts);

    while (nsamples > 0) {
	if (m_input_frames == 0 && !m_end_of_input) {
	    m_input_frames =
		readSamplesAsFloat(source(), &m_ibuffer,
				   &m_fbuffer[1], nsamples);
	    if (!m_input_frames)
		m_end_of_input = true;
	    src = &m_fbuffer[1][0];
	    for (size_t i = 0; i < m_input_frames; ++i) {
		std::vector<uint32_t>::iterator it = m_pass_channels.begin();
		for (; it != m_pass_channels.end(); ++it)
		    m_syncque.push_back(src[i * ichannels + *it]);
		it = m_shift_channels.begin();
		for (; it != m_shift_channels.end(); ++it)
		    src[i * ichannels + *it] /= m_filter_gain;
	    }
	}
	uint32_t ilen = m_input_frames;
	uint32_t olen = nsamples;
	size_t j = 0;
	for (size_t i = 0; i < m_shift_channels.size(); ++i) {
	    ivec[i] = src + m_shift_channels[i];
	    ovec[i] = dst + m_shift_channels[i];
	}
	m_module.fir_process(m_filter.get(), &ivec[0], &ovec[0],
			     &ilen, &olen, ichannels, ichannels);
	for (size_t i = 0; i < olen; ++i) {
	    std::vector<uint32_t>::iterator it = m_pass_channels.begin();
	    if (m_syncque.size()) {
		for (; it != m_pass_channels.end(); ++it) {
		    dst[i * ichannels + *it] = m_syncque.front();
		    m_syncque.pop_front();
		}
	    } else {
		for (; it != m_pass_channels.end(); ++it)
		    dst[i * ichannels + *it] = 0.0;
	    }
	}
	nsamples -= olen;
	m_input_frames -= ilen;
	src += ilen * ichannels;
	if (m_end_of_input && olen == 0)
	    break;
	dst += olen * ichannels;
    }
    if (m_input_frames) {
	std::memmove(&this->m_fbuffer[1][0], src,
		     m_input_frames * ichannels * sizeof(float));
    }
    return (dst - static_cast<float*>(buffer)) / ichannels;
}
