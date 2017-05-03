#ifndef SOXLPF_H
#define SOXLPF_H

#include "SoxConvolverModule.h"
#include "FilterBase.h"
#include "util.h"

class SoxLowpassFilter: public FilterBase {
    int64_t m_position;
    std::vector<uint8_t > m_pivot;
    util::FIFO<float> m_buffer;
    std::shared_ptr<lsx_convolver_t> m_convolver;
    AudioStreamBasicDescription m_asbd;
    SoXConvolverModule m_module;
public:
    SoxLowpassFilter(const SoXConvolverModule &module,
                     const std::shared_ptr<ISource> &src,
                     unsigned Fp);
    ~SoxLowpassFilter() { m_convolver.reset(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    int64_t getPosition() { return m_position; }
};

#endif
