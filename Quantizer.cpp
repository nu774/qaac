#include "Quantizer.h"

template <typename T>
inline T clip(T x, T min, T max)
{
    if (x > max) x = max;
    else if (x < min) x = min;
    return x;
}

/* inacculate, but much faster than std::uniform_int_distribution<> */
template <typename Engine>
class RandomIntSpan {
    Engine &e_;
    int div_, off_;
public:
    RandomIntSpan(Engine &e, int min, int max)
	: e_(e), off_(min)
    {
	div_ = e.max() / (max - min);
    }
    int operator()() { return e_() / div_ + off_; }
};

Quantizer::Quantizer(const std::shared_ptr<ISource> &source,
		     uint32_t bitdepth, bool no_dither, bool is_float)
    : FilterBase(source), m_no_dither(no_dither)
{
    const AudioStreamBasicDescription &asbd = source->getSampleFormat();
    m_asbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
				      asbd.mChannelsPerFrame,
				      bitdepth, 32,
				      is_float ? kAudioFormatFlagIsFloat
				      	: kAudioFormatFlagIsSignedInteger);
}

size_t Quantizer::readSamples(void *buffer, size_t nsamples)
{
    const AudioStreamBasicDescription &iasbd = source()->getSampleFormat();

    if (m_asbd.mFormatFlags & kAudioFormatFlagIsFloat) {
	float *fp = static_cast<float*>(buffer);
	nsamples = readSamplesAsFloat(source(), &m_ibuffer, fp, nsamples);
    } else if (iasbd.mFormatFlags & kAudioFormatFlagIsSignedInteger) {
	nsamples = source()->readSamples(buffer, nsamples);
	if (m_asbd.mBitsPerChannel < iasbd.mBitsPerChannel) {
	    ditherInt(static_cast<int*>(buffer),
		      nsamples * m_asbd.mChannelsPerFrame,
		      m_asbd.mBitsPerChannel);
	}
    } else if (iasbd.mBitsPerChannel <= 32) {
	nsamples = readSamplesAsFloat(source(), 0, &m_fbuffer,
				      nsamples);
	ditherFloat(&m_fbuffer[0], static_cast<int*>(buffer),
		    nsamples * m_asbd.mChannelsPerFrame,
		    m_asbd.mBitsPerChannel);
    } else {
	nsamples = readSamplesAsFloat(source(), 0, &m_dbuffer,
				      nsamples);
	ditherFloat(&m_dbuffer[0], static_cast<int*>(buffer),
		    nsamples * m_asbd.mChannelsPerFrame,
		    m_asbd.mBitsPerChannel);
    }
    return nsamples;
}

/*
 *  MSB <-------------------------> LSB
 *  <----------- original ------------>
 *  <------ converted ------->
 *  xxxxxxxx xxxxxxxx xxxxxxxx yyyyyyyy
 *
 *  We regard this as 24.7 fixed point num, and round to 24bit int.
 *  (We truncate 1bit from LSB side for handling overflow/saturation)
 */
void Quantizer::ditherInt(int *data, size_t count, unsigned depth)
{
    const int one = 1 << (31 - depth);
    const int half = one / 2;
    const unsigned mask = ~(one - 1);
    /*
     * XXX:
     * std::uniform_int_distribution<int> is too slow due to
     * double<->int conversion
     */
    // std::uniform_int_distribution<int> dist(-half, half);
    RandomIntSpan<std::mt19937> nrand(m_mt, -half, half);
    for (size_t i = 0; i < count; ++i) {
	int value = data[i] >> 1;
	if (depth <= 18 && !m_no_dither) {
	    // int noise = dist(m_mt) + dist(m_mt);
	    int noise = nrand() + nrand();
	    value += noise;
	}
	data[i] = (clip(value + half, INT_MIN >> 1, INT_MAX >> 1) & mask)<< 1;
    }
}

template <typename T>
void Quantizer::ditherFloat(T *src, int *dst, size_t count, unsigned depth)
{
    int shifts = 32 - depth;
    double half = 1U << (depth - 1);
    double min_value = -half;
    double max_value = half - 1;
    std::uniform_real_distribution<double> dist(-0.5, 0.5);
    for (size_t i = 0; i < count; ++i) {
	double value = src[i] * half;
	if (depth <= 18 && !m_no_dither) {
	    double noise = dist(m_mt) + dist(m_mt);
	    value += noise;
	}
	dst[i] = lrint(clip(value, min_value, max_value)) << shifts;
    }
}

