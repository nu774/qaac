#ifndef STREAMINGCONVOLVER_H
#define STREAMINGCONVOLVER_H

#include <cstddef>
#include <cstdint>
#include <vector>
#include "util.h"

/*
 * Streaming single-channel FIR filtering via overlap-save block
 * convolution (FFT-based fast convolution, see e.g. the "Overlap-save
 * method" article for the general technique).
 *
 * The filter is delay-compensated: priming construction with `postPeak`
 * (typically the index of the peak/center tap, i.e. the filter's group
 * delay in samples) means that, once enough samples have flowed through,
 * output sample i corresponds to input sample i rather than being
 * shifted by the filter's inherent latency.
 */
class StreamingConvolver {
public:
    StreamingConvolver(const std::vector<double> &coefs, size_t postPeak);

    /*
     * Feeds *ilen input samples (istride floats apart, starting at ibuf)
     * and writes up to *olen output samples (ostride floats apart,
     * starting at obuf). All offered input is always consumed; *olen is
     * updated to the number of samples actually written (which may be
     * fewer than requested, including zero, if not enough filtered data
     * is available yet).
     *
     * Passing *ilen == 0 marks end of stream: remaining delayed samples
     * are flushed (padding internally with silence as needed) until the
     * total number of samples ever delivered equals the total number of
     * real samples fed via earlier calls. Once called this way, no
     * further calls with *ilen > 0 should be made.
     */
    void process(const float *ibuf, float *obuf,
                size_t istride, size_t ostride,
                size_t *ilen, size_t *olen);

private:
    void feed(const float *ibuf, size_t n, size_t stride);
    void runBlocks(size_t maxBuffered);
    void convolveBlock(float *block);

    size_t m_numTaps;
    size_t m_dftLength;
    size_t m_blockAdvance;        /* new samples consumed per FFT block */
    std::vector<float> m_coefsFreq;
    std::vector<int> m_fftIp;
    std::vector<float> m_fftW;
    std::vector<float> m_scratch; /* one block, working buffer */

    util::FIFO<float> m_pending;  /* input awaiting block processing */
    util::FIFO<float> m_output;   /* filtered samples awaiting delivery */

    uint64_t m_realSamplesIn;
    uint64_t m_totalDelivered;
    bool m_finished;
};

#endif
