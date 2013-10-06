#ifndef INTEGER_SOURCE_H
#define INTEGER_SOURCE_H

#include <assert.h>
#include <random>
#include "iointer.h"
#include "cautil.h"
#include "rng.h"

class Quantizer: public FilterBase {
    typedef rng::LCG RandomEngine;
    AudioStreamBasicDescription m_asbd;
    RandomEngine m_engine;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float> m_fbuffer;
    std::vector<double> m_dbuffer;
    bool m_no_dither;
public:
    Quantizer(const std::shared_ptr<ISource> &source, uint32_t bitdepth,
              bool no_dither, bool is_float=false);
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
private:
    void ditherInt(int *data, size_t count, unsigned depth);
    template <typename T>
    void ditherFloat(T *src, int *dst, size_t count, unsigned depth);
};

#endif
