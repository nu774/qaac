#ifndef GAIN_H
#define GAIN_H

#include <cmath>
#include "iointer.h"

inline double dB_to_scale(double dB)
{
    return std::pow(10, 0.05 * dB);
}

class Scaler: public DelegatingSource {
    double m_scale;
    std::vector<uint8_t> m_ibuffer;
    SampleFormat m_format;
public:
    Scaler(const x::shared_ptr<ISource> &source, double scale)
	: DelegatingSource(source), m_scale(scale)
    {
	const SampleFormat &fmt = source->getSampleFormat();
	if (fmt.m_bitsPerSample == 64)
	    throw std::runtime_error("Can't handle 64bit sample");
	m_format = SampleFormat("F32LE", fmt.m_nchannels, fmt.m_rate);
    }
    const SampleFormat &getSampleFormat() const { return m_format; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	float *fp = static_cast<float*>(buffer);
	size_t nc = readSamplesAsFloat(source(), &m_ibuffer, fp, nsamples);
	size_t len = nc * source()->getSampleFormat().m_nchannels;
	for (size_t i = 0; i < len; ++i)
	    fp[i] *= m_scale;
	return nc;
    }
};

#endif
