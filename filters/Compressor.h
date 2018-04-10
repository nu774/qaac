#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <deque>
#include "FilterBase.h"
#include "util.h"
#include "WaveSink.h"

class Compressor: public FilterBase {
    const double m_threshold;
    const double m_slope;
    const double m_attack;
    const double m_release;

    const double m_Tlo;
    const double m_Thi;
    const double m_knee_factor;

    double m_yR;
    double m_yA;
    bool m_eof;
    int64_t m_position;
    std::vector<uint8_t > m_pivot;
    util::FIFO<float> m_buffer;
    std::deque<std::pair<int64_t, float>> m_window;
    AudioStreamBasicDescription m_asbd;
    std::shared_ptr<FILE> m_statfile;
    std::shared_ptr<WaveSink> m_statsink;
    std::vector<float> m_statbuf;
public:
    Compressor(const std::shared_ptr<ISource> &src,
               double threshold, double ratio, double knee_width,
               double attack, double release,
               std::shared_ptr<FILE> statfp);
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
private:
    float getPeakValue(const float *data, unsigned i, unsigned nchannels,
                       unsigned lookahead);
    /*
     * gain computer, works on log domain
     */
    double computeGain(double x)
    {
        if (x < m_Tlo)
            return 0.0;
        else if (x > m_Thi)
            return m_slope * (x - m_threshold);
        else {
            double delta = x - m_Tlo;
            return delta * delta * m_knee_factor;
        }
    }
    /*
     * smooth, level corrected decoupled peak detector
     * works on log domain
     */
    double smoothAverage(double x, double alphaA, double alphaR)
    {
        const double eps = 1e-120;
        m_yR = std::min(x, alphaR * m_yR + (1.0 - alphaR) * x + eps - eps);
        m_yA = alphaA * m_yA + (1.0 - alphaA) * m_yR + eps - eps;
        return m_yA;
    }
};

#endif
