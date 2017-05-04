#ifndef CoreAudioPaddedEncoder_H
#define CoreAudioPaddedEncoder_H

#include "util.h"
#include "CoreAudioEncoder.h"

class CoreAudioPaddedEncoder: public CoreAudioEncoder {
    enum { LPC_ORDER = 32 };
    enum { APPLE_NUM_PRIMING = 2112 };
    util::FIFO<float> m_buffer;
    std::vector<uint8_t> m_pivot;
    std::vector<uint8_t> m_frame;
    unsigned m_num_priming;
    size_t m_frames;
    size_t (CoreAudioPaddedEncoder::*m_read)(void *, size_t);
    void (CoreAudioPaddedEncoder::*m_write)(const void *, size_t, size_t);
public:
    CoreAudioPaddedEncoder(AudioConverterXX &converter,
                           uint32_t num_priming=2112);
    AudioFilePacketTableInfo getGaplessInfo();
protected:
    size_t readSamples(void *buffer, size_t nsamples)
    {
        return (this->*m_read)(buffer, nsamples);
    }
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        (this->*m_write)(data, length, nsamples);
    }
private:
    void reverse_buffer(float *data, unsigned nsamples, unsigned nchannels);
    void extrapolate(float *input, size_t ilen,
                     float *output, size_t olen);
    void extrapolate0();
    void extrapolate1();
    size_t readSamples0(void *buffer, size_t nsamples);
    size_t readSamples1(void *buffer, size_t nsamples);
    void writeSamplesLC(const void *data, size_t length, size_t nsamples);
    void writeSamplesHE(const void *data, size_t length, size_t nsamples);
};
#endif
