#ifndef PEAKSINK_H
#define PEAKSINK_H

class PeakSink: public ISink {
    double m_peak;
    double m_scale;
    AudioStreamBasicDescription m_asbd;
public:
    PeakSink(const AudioStreamBasicDescription &asbd)
        : m_peak(0.0), m_scale(1.0), m_asbd(asbd)
    {
        if (m_asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger)
            m_scale = 2147483648.0;
    }
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        nsamples *= m_asbd.mChannelsPerFrame;
        if (m_asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger)
            process(static_cast<const int32_t *>(data), nsamples);
        else if (m_asbd.mBitsPerChannel == 32)
            process(static_cast<const float *>(data), nsamples);
        else
            process(static_cast<const double *>(data), nsamples);
    }
    double peak() const
    {
        return m_peak / m_scale;
    }
private:
    template <typename T>
    void process(const T *data, size_t count)
    {
        for (size_t i = 0; i < count; ++i) {
            double x = std::abs(static_cast<double>(data[i]));
            if (x > m_peak) m_peak = x;
        }
    }
};

#endif
