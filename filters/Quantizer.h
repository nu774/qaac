#ifndef INTEGER_SOURCE_H
#define INTEGER_SOURCE_H

#include <assert.h>
#include <random>
#include "FilterBase.h"
#include "cautil.h"
#include "rng.h"

class Quantizer: public FilterBase {
    typedef rng::LCG RandomEngine;
    AudioStreamBasicDescription m_asbd;
    RandomEngine m_engine;
    std::vector<uint8_t> m_pivot;
    size_t (Quantizer::*m_convert)(void *buffer, size_t nsamples);
public:
    Quantizer(const std::shared_ptr<ISource> &source, uint32_t bitdepth,
              bool no_dither, bool is_float=false);
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
        return (this->*m_convert)(buffer, nsamples);
    }
private:
    size_t convertSamples_a2f(void *buffer, size_t nsamples);
    size_t convertSamples_i2i_0(void *buffer, size_t nsamples);
    size_t convertSamples_i2i_1(void *buffer, size_t nsamples);
    size_t convertSamples_i2i_2(void *buffer, size_t nsamples);
    size_t convertSamples_h2i_1(void *buffer, size_t nsamples);
    size_t convertSamples_h2i_2(void *buffer, size_t nsamples);
    size_t convertSamples_f2i_1(void *buffer, size_t nsamples);
    size_t convertSamples_f2i_2(void *buffer, size_t nsamples);
    size_t convertSamples_d2i_1(void *buffer, size_t nsamples);
    size_t convertSamples_d2i_2(void *buffer, size_t nsamples);

    void ditherInt1(int32_t *dst, size_t count, unsigned bits);
    void ditherInt2(int32_t *dst, size_t count, unsigned bits);
    template <typename T>
    void ditherFloat1(const T *src, int *dst, size_t count, unsigned bits);
    template <typename T>
    void ditherFloat2(const T *src, int *dst, size_t count, unsigned bits);

    void growPivot(size_t nsamples);
};

#endif
