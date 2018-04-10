#include <numeric>
#include "Compressor.h"
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
      m_eof(false),
      m_position(0),
      m_statfile(statfp)
{
    const AudioStreamBasicDescription &asbd = src->getSampleFormat();
    m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate, asbd.mChannelsPerFrame,
                                     32, kAudioFormatFlagIsFloat);
    m_buffer.set_unit(asbd.mChannelsPerFrame);
    if (m_statfile.get()) {
        AudioStreamBasicDescription asbd =
            cautil::buildASBDForPCM(m_asbd.mSampleRate, 1,
                                    32, kAudioFormatFlagIsFloat);
        m_statsink = std::make_shared<WaveSink>(m_statfile, ~0ULL, asbd);
    }
}

size_t Compressor::readSamples(void *buffer, size_t nsamples)
{
    const double Fs = m_asbd.mSampleRate;
    unsigned nchannels = m_asbd.mChannelsPerFrame;
    const double alphaA = 
        m_attack > 0.0 ? std::exp(-1.0 / (m_attack * Fs)) : 0.0;
    const double alphaR =
        m_release > 0.0 ? std::exp(-1.0 / (m_release * Fs)) : 0.0;
    unsigned lookahead = m_attack * Fs + .5;
    uint32_t bpf = getSampleFormat().mBytesPerFrame;

    while (!m_eof && m_buffer.count() < nsamples + lookahead) {
        size_t count = lookahead + nsamples - m_buffer.count();
        m_buffer.reserve(count);
        size_t nr = readSamplesAsFloat(source(), &m_pivot,
                                       m_buffer.write_ptr(), count);
        m_buffer.commit(nr);
        if (!nr) {
            m_buffer.reserve(lookahead);
            memset(m_buffer.write_ptr(), 0, lookahead * bpf);
            m_buffer.commit(lookahead);
            m_eof = true;
        }
    }
    nsamples = std::min(nsamples, m_buffer.count() - lookahead);
    float *data = m_buffer.read(nsamples);

    if (m_statbuf.size() < nsamples)
        m_statbuf.resize(nsamples);

    for (size_t i = 0; i < nsamples; ++i) {
        float xL = getPeakValue(data, i, nchannels, lookahead);
        double xG = util::scale_to_dB(xL);
        double yG = computeGain(xG);
        double cG = smoothAverage(yG, alphaA, alphaR);
        float cL = static_cast<float>(util::dB_to_scale(cG));
        for (unsigned n = 0; n < nchannels; ++n)
            data[i * nchannels + n] *= cL;
        m_statbuf[i] = cL;
    }
    memcpy(buffer, data, nsamples * bpf);
    if (m_statsink.get()) {
        m_statsink->writeSamples(m_statbuf.data(), nsamples * sizeof(float),
                                 nsamples);
        if (nsamples == 0)
            m_statsink->finishWrite();
    }
    m_position += nsamples;
    return nsamples;
}

float Compressor::getPeakValue(const float *data, unsigned i,
                               unsigned nchannels, unsigned lookahead)
{
    if (!lookahead)
        return frame_amplitude(&data[i * nchannels], nchannels);
    if (m_window.empty()) {
        for (unsigned k = 0; k < lookahead; ++k) {
            float x = frame_amplitude(&data[k * nchannels], nchannels);
            while (!m_window.empty() && x >= m_window.back().second)
                m_window.pop_back();
            m_window.push_back(std::make_pair((int64_t)k, x));
        }
    }
    float res = m_window.front().second;
    while (!m_window.empty() && m_window.front().first <= m_position + i)
        m_window.pop_front();
    float x = frame_amplitude(&data[(i + lookahead) * nchannels], nchannels);
    while (!m_window.empty() && x >= m_window.back().second)
        m_window.pop_back();
    m_window.push_back(std::make_pair(m_position + i + lookahead, x));
    return res;
}
