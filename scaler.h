#ifndef GAIN_H
#define GAIN_H

#include <cmath>
#include "iointer.h"

class Scaler: public FilterBase {
    double m_scale;
    std::vector<uint8_t> m_ibuffer;
    AudioStreamBasicDescription m_asbd;
public:
    Scaler(const std::shared_ptr<ISource> &source, double scale)
        : FilterBase(source), m_scale(scale)
    {
        const AudioStreamBasicDescription &asbd = source->getSampleFormat();
        unsigned bits = 32;
        if (asbd.mBitsPerChannel > 32
            || (asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger) &&
               asbd.mBitsPerChannel > 24)
            bits = 64;

        m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate,
                                         asbd.mChannelsPerFrame,
                                         bits, kAudioFormatFlagIsFloat);
    }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    template <typename T>
    size_t readSamplesT(T *buffer, size_t nsamples)
    {
        size_t nc = readSamplesAsFloat(source(), &m_ibuffer, buffer, nsamples);
        size_t len = nc * source()->getSampleFormat().mChannelsPerFrame;
        for (size_t i = 0; i < len; ++i)
            buffer[i] *= m_scale;
        return nc;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
        if (m_asbd.mBitsPerChannel == 64)
            return readSamplesT(static_cast<double*>(buffer), nsamples);
        else
            return readSamplesT(static_cast<float*>(buffer), nsamples);
    }
};

#endif
