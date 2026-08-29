#ifndef HALFBANDINTERPOLATOR_H
#define HALFBANDINTERPOLATOR_H

#include <memory>
#include <vector>
#include "StreamingConvolver.h"
#include "FilterBase.h"
#include "util.h"

class HalfbandInterpolator: public FilterBase {
public:
    explicit HalfbandInterpolator(const std::shared_ptr<ISource> &src);
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
    util::FIFO<float> m_zstuffed; /* zero-stuffed, at the doubled rate */
    util::FIFO<float> m_filtered;
    std::vector<uint8_t> m_pivot;
    std::vector<float> m_scratch;
    bool m_eof;
    int64_t m_position;
    uint64_t m_length;
    ca::AudioStreamBasicDescription m_asbd;
};

#endif
