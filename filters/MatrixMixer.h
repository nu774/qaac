#ifndef MIXER_H
#define MIXER_H

#include <complex>
#include <deque>
#include <memory>
#include "FilterBase.h"
#include "StreamingConvolver.h"
#include "misc.h"
#include "util.h"

class MatrixMixer: public FilterBase {
    typedef misc::complex_t complex_t;
    int64_t m_position;
    std::vector<std::vector<complex_t> > m_matrix;
    std::vector<std::unique_ptr<StreamingConvolver> > m_filter;
    std::vector<unsigned> m_shift_channels, m_pass_channels;
    std::deque<float> m_syncque;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float> m_fbuffer;
    util::FIFO<float> m_buffer;
    ca::AudioStreamBasicDescription m_asbd;
public:
    MatrixMixer(const std::shared_ptr<ISource> &source,
                const std::vector<std::vector<complex_t> > &spec,
                bool normalize=true);
    const ca::AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    int64_t getPosition() { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples);
private:
    void initFilter();
    size_t phaseShift(size_t nsamples);
};

#endif
