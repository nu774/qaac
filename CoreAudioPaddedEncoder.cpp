#include "CoreAudioPaddedEncoder.h"
extern "C" {
#include "lpc.h"
}

CoreAudioPaddedEncoder::CoreAudioPaddedEncoder(AudioConverterXX &converter,
                                               uint32_t num_priming)
    : CoreAudioEncoder(converter),
      m_num_priming(num_priming),
      m_frames(0),
      m_read(&CoreAudioPaddedEncoder::readSamples0)
{
    m_buffer.set_unit(getInputDescription().mChannelsPerFrame);
    if (getOutputDescription().mFormatID == 'aac ')
        m_write = &CoreAudioPaddedEncoder::writeSamplesLC;
    else
        m_write = &CoreAudioPaddedEncoder::writeSamplesHE;
}
AudioFilePacketTableInfo CoreAudioPaddedEncoder::getGaplessInfo()
{
    AudioFilePacketTableInfo pinfo = CoreAudioEncoder::getGaplessInfo();
    if (getOutputDescription().mFormatID == 'aach') {
        unsigned fpp = getOutputDescription().mFramesPerPacket / 2;
        pinfo.mNumberValidFrames -= fpp;
        pinfo.mRemainderFrames += fpp;
    } else {
        pinfo.mPrimingFrames = m_num_priming;
        pinfo.mNumberValidFrames += (APPLE_NUM_PRIMING - m_num_priming);
    }
    return pinfo;
}
void CoreAudioPaddedEncoder::reverse_buffer(float *data, unsigned nsamples,
                                            unsigned nchannels)
{
    unsigned i = 0, j = nchannels * (nsamples - 1), n;
    for (; i < j; i += nchannels, j -= nchannels) {
        for (n = 0; n < nchannels; ++n) {
            float tmp = data[i + n];
            data[i + n] = data[j + n];
            data[j + n] = tmp;
        }
    }
}
void CoreAudioPaddedEncoder::extrapolate(float *input, size_t ilen,
                                         float *output, size_t olen)
{
    unsigned n = getInputDescription().mChannelsPerFrame;
    float lpc[LPC_ORDER];
    for (unsigned i = 0; i < n; ++i) {
        vorbis_lpc_from_data(input + i, lpc, ilen, LPC_ORDER, n);
        vorbis_lpc_predict(lpc, &input[i + n * (ilen - LPC_ORDER)],
                           LPC_ORDER, output + i, olen, n);
    }
}
void CoreAudioPaddedEncoder::extrapolate0()
{
    unsigned nchannels = getInputDescription().mChannelsPerFrame;
    unsigned bpf = getInputDescription().mBytesPerFrame;
    unsigned fpp = getOutputDescription().mFramesPerPacket;
    unsigned nsamples = fpp / 2;
    unsigned padding = (getOutputDescription().mFormatID == 'aac ')
        ? m_num_priming + 3 * fpp - APPLE_NUM_PRIMING - nsamples
        : nsamples;

    std::vector<float> buf(nsamples * nchannels);
    size_t n = readSamplesFull(src(), &buf[0], nsamples);
    m_buffer.reserve(nsamples + n + padding);
    if (padding > 0) {
        std::memset(m_buffer.write_ptr(), 0, padding * bpf);
        m_buffer.commit(padding);
    }
    if (n < 2 * LPC_ORDER)
        std::memset(m_buffer.write_ptr(), 0, nsamples * bpf);
    else {
        reverse_buffer(&buf[0], n, nchannels);
        extrapolate(&buf[0], n, m_buffer.write_ptr(), nsamples);
        reverse_buffer(m_buffer.write_ptr(), nsamples, nchannels);
        reverse_buffer(&buf[0], n, nchannels);
    }
    m_buffer.commit(nsamples);
    std::copy(buf.begin(), buf.begin() + n * nchannels,
              m_buffer.write_ptr());
    m_buffer.commit(n);
}
void CoreAudioPaddedEncoder::extrapolate1()
{
    unsigned fpp = getOutputDescription().mFramesPerPacket;
    unsigned bpf = getInputDescription().mBytesPerFrame;
    size_t count = m_buffer.count();
    m_buffer.reserve(fpp);
    if (count >= 2 * LPC_ORDER)
        extrapolate(m_buffer.read_ptr(), count, m_buffer.write_ptr(), fpp);
    else
        std::memset(m_buffer.write_ptr(), 0, fpp * bpf);
    m_buffer.commit(fpp);
}
size_t CoreAudioPaddedEncoder::readSamples0(void *buffer, size_t nsamples)
{
    size_t nread = 0;
    unsigned fpp = getOutputDescription().mFramesPerPacket;
    unsigned bpf = getInputDescription().mBytesPerFrame;

    if (samplesRead() == 0)
        extrapolate0();
    int need_samples = nsamples + fpp - m_buffer.count();
    if (need_samples > 0) {
        m_buffer.reserve(need_samples);
        nread = readSamplesFull(src(), m_buffer.write_ptr(), need_samples);
        m_buffer.commit(nread);
        if (nread == 0) {
            extrapolate1();
            m_read = &CoreAudioPaddedEncoder::readSamples1;
            return readSamples1(buffer, nsamples);
        }
    }
    size_t n = std::min(nsamples,
                        static_cast<size_t>(m_buffer.count() - fpp));
    std::memcpy(buffer, m_buffer.read(n), n * bpf);
    return n;
}
size_t CoreAudioPaddedEncoder::readSamples1(void *buffer, size_t nsamples)
{
    size_t n = std::min(nsamples, m_buffer.count());
    if (n > 0)
        std::memcpy(buffer, m_buffer.read(n),
                    n * getInputDescription().mBytesPerFrame);
    return n;
}
void CoreAudioPaddedEncoder::writeSamplesLC(const void *data, size_t length,
                                            size_t nsamples)
{
    if (m_frame.size() && ++m_frames > 3)
        CoreAudioEncoder::writeSamples(&m_frame[0], m_frame.size(),
                                       nsamples);
    m_frame.resize(length);
    std::memcpy(&m_frame[0], data, length);
}
void CoreAudioPaddedEncoder::writeSamplesHE(const void *data, size_t length,
                                            size_t nsamples)
{
    if (++m_frames != 2)
        CoreAudioEncoder::writeSamples(data, length, nsamples);
}
