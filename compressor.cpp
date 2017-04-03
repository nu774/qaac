#include <numeric>
#include "compressor.h"
#include "cautil.h"

namespace {
    template <typename T>
    inline T frame_amplitude(const T *frame, unsigned nchannels)
    {
        return std::accumulate(frame, frame + nchannels, static_cast<T>(0),
                               [](T acc, T x) -> T {
                                  return std::max(acc, std::abs(x));
                               });
    }
}

Compressor::Compressor(const std::shared_ptr<ISource> &src,
                       double threshold, double ratio, double knee_width,
                       double attack, double release,
                       std::shared_ptr<FILE> statfp)
    : FilterBase(src),
      m_threshold(threshold),
      m_slope((1.0 - ratio) / ratio),
      m_attack(attack / 1000.0),
      m_release(release / 1000.0),
      m_Tlo(threshold - knee_width / 2.0),
      m_Thi(threshold + knee_width / 2.0),
      m_knee_factor(m_slope / (knee_width * 2.0)),
      m_yR(0.0),
      m_yA(0.0),
      m_statfile(statfp)
{
    const AudioStreamBasicDescription &asbd = src->getSampleFormat();
    unsigned bits = 32;
    if (asbd.mBitsPerChannel > 32
        || ((asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger) &&
            asbd.mBitsPerChannel > 24))
        bits = 64;
    m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate, asbd.mChannelsPerFrame,
                                     bits, kAudioFormatFlagIsFloat);
    if (m_statfile.get()) {
        AudioStreamBasicDescription asbd =
            cautil::buildASBDForPCM(m_asbd.mSampleRate, 1,
                                    32, kAudioFormatFlagIsFloat);
        m_statsink = std::make_shared<WaveSink>(m_statfile.get(), ~0ULL, asbd);
    }
}

size_t Compressor::readSamples(void *buffer, size_t nsamples)
{
    if (m_asbd.mBitsPerChannel == 64)
        return readSamplesT(static_cast<double*>(buffer), nsamples);
    else
        return readSamplesT(static_cast<float*>(buffer), nsamples);
}

template <typename T>
size_t Compressor::readSamplesT(T *buffer, size_t nsamples)
{
    const double Fs = m_asbd.mSampleRate;
    unsigned nchannels = m_asbd.mChannelsPerFrame;
    const double alphaA = 
        m_attack > 0.0 ? std::exp(-1.0 / (m_attack * Fs)) : 0.0;
    const double alphaR =
        m_release > 0.0 ? std::exp(-1.0 / (m_release * Fs)) : 0.0;

    nsamples = readSamplesAsFloat(source(), &m_pivot, buffer, nsamples);

    if (m_statbuf.size() < nsamples)
        m_statbuf.resize(nsamples);

    for (size_t i = 0; i < nsamples; ++i) {
        T *frame = &buffer[i * nchannels];
        double xL = frame_amplitude(frame, nchannels);
        double xG = util::scale_to_dB(xL);
        double yG = computeGain(xG);
        double cG = smoothAverage(yG, alphaA, alphaR);
        T cL = static_cast<T>(util::dB_to_scale(cG));
        for (unsigned n = 0; n < nchannels; ++n)
            frame[n] *= cL;
        m_statbuf[i] = cL;
    }
    if (m_statsink.get()) {
        m_statsink->writeSamples(m_statbuf.data(), nsamples * sizeof(float),
                                 nsamples);
        if (nsamples == 0)
            m_statsink->finishWrite();
    }
    return nsamples;
}
