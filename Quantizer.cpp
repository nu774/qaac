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

template <typename Engine>
class RandomIntSpanShift {
    Engine &e_;
    int shift_, off_;
public:
    RandomIntSpanShift(Engine &e, int log2)
        : e_(e)
    {
        shift_ = 32 - log2;
        off_ = - (1 << (log2 - 1));
    }
    int operator()() { return (e_() >> shift_) + off_; }
};

Quantizer::Quantizer(const std::shared_ptr<ISource> &source,
                     uint32_t bitdepth, bool no_dither, bool is_float)
    : FilterBase(source)
{
    const AudioStreamBasicDescription &asbd = source->getSampleFormat();
    m_asbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
                                      asbd.mChannelsPerFrame,
                                      bitdepth, 32,
                                      is_float ? kAudioFormatFlagIsFloat
                                        : kAudioFormatFlagIsSignedInteger);

    bool dither = !no_dither && m_asbd.mBitsPerChannel <= 18;

    if (m_asbd.mFormatFlags & kAudioFormatFlagIsFloat)
        m_convert = &Quantizer::convertSamples_a2f;
    else if (asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger) {
        if (m_asbd.mBitsPerChannel >= asbd.mBitsPerChannel)
            m_convert = &Quantizer::convertSamples_i2i_0;
        else if (dither)
            m_convert = &Quantizer::convertSamples_i2i_2;
        else
            m_convert = &Quantizer::convertSamples_i2i_1;
    } else if (asbd.mBitsPerChannel <= 32)
        m_convert = dither ? &Quantizer::convertSamples_f2i_2
                           : &Quantizer::convertSamples_f2i_1;
    else
        m_convert = dither ? &Quantizer::convertSamples_d2i_2
                           : &Quantizer::convertSamples_d2i_1;
}

size_t Quantizer::convertSamples_a2f(void *buffer, size_t nsamples)
{
    return readSamplesAsFloat(source(), &m_pivot,
                              static_cast<float*>(buffer), nsamples);
}

size_t Quantizer::convertSamples_i2i_0(void *buffer, size_t nsamples)
{
    return source()->readSamples(buffer, nsamples);
}

size_t Quantizer::convertSamples_i2i_1(void *buffer, size_t nsamples)
{
    nsamples = source()->readSamples(buffer, nsamples);
    ditherInt1(static_cast<int32_t *>(buffer),
               m_asbd.mChannelsPerFrame * nsamples,
               m_asbd.mBitsPerChannel);
    return nsamples;
}

size_t Quantizer::convertSamples_i2i_2(void *buffer, size_t nsamples)
{
    nsamples = source()->readSamples(buffer, nsamples);
    ditherInt2(static_cast<int32_t *>(buffer),
               m_asbd.mChannelsPerFrame * nsamples,
               m_asbd.mBitsPerChannel);
    return nsamples;
}

size_t Quantizer::convertSamples_f2i_1(void *buffer, size_t nsamples)
{
    nsamples = source()->readSamples(buffer, nsamples);
    ditherFloat1(static_cast<float *>(buffer),
                 static_cast<int32_t *>(buffer),
                 m_asbd.mChannelsPerFrame * nsamples,
                 m_asbd.mBitsPerChannel);
    return nsamples;
}

size_t Quantizer::convertSamples_f2i_2(void *buffer, size_t nsamples)
{
    nsamples = source()->readSamples(buffer, nsamples);
    ditherFloat2(static_cast<float *>(buffer),
                 static_cast<int32_t *>(buffer),
                 m_asbd.mChannelsPerFrame * nsamples,
                 m_asbd.mBitsPerChannel);
    return nsamples;
}

size_t Quantizer::convertSamples_d2i_1(void *buffer, size_t nsamples)
{
    growPivot(nsamples);
    nsamples = source()->readSamples(&m_pivot[0], nsamples);
    ditherFloat1(reinterpret_cast<double *>(&m_pivot[0]),
                 static_cast<int32_t *>(buffer),
                 m_asbd.mChannelsPerFrame * nsamples,
                 m_asbd.mBitsPerChannel);
    return nsamples;
}

size_t Quantizer::convertSamples_d2i_2(void *buffer, size_t nsamples)
{
    growPivot(nsamples);
    nsamples = source()->readSamples(&m_pivot[0], nsamples);
    ditherFloat2(reinterpret_cast<double *>(&m_pivot[0]),
                 static_cast<int32_t *>(buffer),
                 m_asbd.mChannelsPerFrame * nsamples,
                 m_asbd.mBitsPerChannel);
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
void Quantizer::ditherInt1(int32_t *dst, size_t count, unsigned bits)
{
    const int one = 1 << (31 - bits);
    const int half = one / 2;
    const unsigned mask = ~(one - 1);

    for (size_t i = 0; i < count; ++i) {
        int value = ((dst[i] >> 1) + half) & mask;
        if (value > INT_MAX>>1) value = INT_MAX>>1;
        dst[i] = value << 1;
    }
}

void Quantizer::ditherInt2(int32_t *dst, size_t count, unsigned bits)
{
    const int one = 1 << (31 - bits);
    const int half = one / 2;
    const unsigned mask = ~(one - 1);

    RandomIntSpanShift<RandomEngine> noise(m_engine, 31 - bits);
    for (size_t i = 0; i < count; ++i) {
        int value = (dst[i] >> 1) + half + noise() + noise();
        value &= mask;
        dst[i] = clip(value, INT_MIN>>1, INT_MAX>>1) << 1;
    }
}

template <typename T>
void Quantizer::ditherFloat1(const T *src, int32_t *dst, size_t count,
                             unsigned bits)
{
    int shifts = 32 - bits;
    double half = 1U << (bits - 1);
    double min_value = -half;
    double max_value = half - 1;
    for (size_t i = 0; i < count; ++i) {
        double value = src[i] * half;
        dst[i] = lrint(clip(value, min_value, max_value)) << shifts;
    }
}

template <typename T>
void Quantizer::ditherFloat2(const T *src, int32_t *dst, size_t count,
                             unsigned bits)
{
    int shifts = 32 - bits;
    double half = 1U << (bits - 1);
    double min_value = -half;
    double max_value = half - 1;
    std::uniform_real_distribution<double> dist(-0.5, 0.5);
    for (size_t i = 0; i < count; ++i) {
        double value = src[i] * half;
        double noise = dist(m_engine) + dist(m_engine);
        value += noise;
        dst[i] = lrint(clip(value, min_value, max_value)) << shifts;
    }
}

void Quantizer::growPivot(size_t nsamples)
{
    size_t nbytes = nsamples * source()->getSampleFormat().mBytesPerFrame;
    if (m_pivot.size() < nbytes)
        m_pivot.resize(nbytes);
}
