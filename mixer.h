#ifndef MIXER_H
#define MIXER_H

#include <complex>
#include <deque>
#include "iointer.h"
#include "soxcmodule.h"

typedef std::complex<float> complex_t;

class MatrixMixer: public FilterBase {
    int64_t m_position;
    std::vector<std::vector<complex_t> > m_matrix;
    std::vector<std::shared_ptr<lsx_convolver_t> > m_filter;
    std::vector<unsigned> m_shift_channels, m_pass_channels;
    std::deque<float> m_syncque;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float> m_fbuffer;
    util::FIFO<float> m_buffer;
    AudioStreamBasicDescription m_asbd;
    SoXConvolverModule m_module;
public:
    MatrixMixer(const std::shared_ptr<ISource> &source,
                const SoXConvolverModule &module,
                const std::vector<std::vector<complex_t> > &spec,
                bool normalize=true);
    const AudioStreamBasicDescription &getSampleFormat() const
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
