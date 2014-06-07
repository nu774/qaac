#include <cstring>
#include <vector>
#include "iointer.h"
#include "cautil.h"

class SoftClipper {
    int m_nchannels;
    float m_thresh;
    std::vector<std::vector<float> > m_buffer;
    std::vector<size_t> m_processed;
public:
    explicit SoftClipper(int nchannels, float threshold=0.9921875f)
        : m_nchannels(nchannels), m_thresh(threshold)
    {
        m_buffer.resize(nchannels);
        m_processed.resize(nchannels);
    }
    void process(const float *in, size_t nin, float *out, size_t *nout);
};

class Limiter: public FilterBase {
    SoftClipper m_clipper;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float>   m_fbuffer;
    AudioStreamBasicDescription m_asbd;
public:
    Limiter(const std::shared_ptr<ISource> &source)
        : FilterBase(source),
          m_clipper(source->getSampleFormat().mChannelsPerFrame)
    {
        const AudioStreamBasicDescription &asbd = source->getSampleFormat();
        m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate,
                                         asbd.mChannelsPerFrame, 32,
                                         kAudioFormatFlagIsFloat);
    }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
        if (m_fbuffer.size() < nsamples * m_asbd.mChannelsPerFrame)
            m_fbuffer.resize(nsamples * m_asbd.mChannelsPerFrame);
        size_t nin, nout;
        do {
            nin = readSamplesAsFloat(source(), &m_ibuffer, m_fbuffer.data(),
                                     nsamples);
            nout = nsamples;
            m_clipper.process(m_fbuffer.data(), nin,
                              static_cast<float*>(buffer), &nout);
        } while (nin > 0 && nout == 0);
        return nout;
    }
};

