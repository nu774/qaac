#ifndef HALFBANDDECIMATOR_H
#define HALFBANDDECIMATOR_H

#include <memory>
#include <vector>
#include "StreamingConvolver.h"
#include "FilterBase.h"
#include "util.h"

class HalfbandDecimator: public FilterBase {
public:
    explicit HalfbandDecimator(const std::shared_ptr<ISource> &src);
    uint64_t length() const { return m_length; }
    const ca::AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    int64_t getPosition() { return m_position; }

private:
    bool fillFiltered(size_t minCount);

    unsigned m_nchannels;
    std::vector<std::unique_ptr<StreamingConvolver> > m_convolvers;
    util::FIFO<float> m_raw;      /* pulled from upstream, pre-filter */
    util::FIFO<float> m_filtered; /* filtered, full-rate, pre-decimation */
    std::vector<uint8_t> m_pivot;
    int64_t m_position;
    uint64_t m_length;
    ca::AudioStreamBasicDescription m_asbd;
};

#endif
