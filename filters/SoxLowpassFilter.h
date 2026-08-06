#ifndef SOXLPF_H
#define SOXLPF_H

#include <memory>
#include <vector>
#include "StreamingConvolver.h"
#include "FilterBase.h"
#include "util.h"

class SoxLowpassFilter: public FilterBase {
    int64_t m_position;
    std::vector<uint8_t> m_pivot;
    util::FIFO<float> m_buffer;
    std::vector<std::unique_ptr<StreamingConvolver> > m_convolvers;
    ca::AudioStreamBasicDescription m_asbd;
public:
    SoxLowpassFilter(const std::shared_ptr<ISource> &src, unsigned Fp);
    const ca::AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    int64_t getPosition() { return m_position; }
};

#endif
