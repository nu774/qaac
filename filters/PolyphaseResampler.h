#ifndef POLYPHASERESAMPLER_H
#define POLYPHASERESAMPLER_H

#include <cstdint>
#include <vector>
#include "FilterBase.h"
#include "util.h"

class PolyphaseResampler: public FilterBase {
public:
    PolyphaseResampler(const std::shared_ptr<ISource> &src, unsigned rate);

    uint64_t length() const { return m_length; }
    const ca::AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    int64_t getPosition() { return m_position; }

private:
    bool fill(int64_t throughFrame);

    unsigned m_nchannels;
    size_t m_phases;        /* P: phase table size */
    size_t m_tapsPerPhase;  /* Np: taps per phase */
    int64_t m_windowBase;   /* b: fixed window-base offset (<= 0) */
    std::vector<float> m_table; /* P * Np, phase-major */

    double m_step;          /* Fin / Fout, input frames per output frame */
    int64_t m_bufBaseFrame; /* absolute input-frame index at m_buffer[*].read_ptr() */
    int64_t m_realFramesIn; /* real (non-padding) input frames seen so far */
    bool m_tailPadded;

    std::vector<util::FIFO<float> > m_buffer;
    std::vector<uint8_t> m_pivot;
    std::vector<float> m_scratch;

    int64_t m_position;
    uint64_t m_length;
    ca::AudioStreamBasicDescription m_asbd;
};

#endif
