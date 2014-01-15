#ifndef CoreAudioCookedEncoder_H
#define CoreAudioCookedEncoder_H

#include "CoreAudioEncoder.h"
extern "C" {
#include "lpc.h"
}
class CoreAudioNoDelayEncoder: public CoreAudioEncoder {
    size_t m_leading, m_frames;
    size_t (CoreAudioNoDelayEncoder::*m_read)(void *, size_t);
    void (CoreAudioNoDelayEncoder::*m_write)(const void *, size_t, size_t);
public:
    CoreAudioNoDelayEncoder(AudioConverterX &converter)
        : CoreAudioEncoder(converter),
          m_leading(1024 * 3 - 2112),
          m_frames(0),
          m_read(&CoreAudioNoDelayEncoder::readSamples0),
          m_write(&CoreAudioNoDelayEncoder::writeSamples0)
    {}
    AudioFilePacketTableInfo getGaplessInfo()
    {
        AudioFilePacketTableInfo pinfo = CoreAudioEncoder::getGaplessInfo();
        pinfo.mNumberValidFrames += pinfo.mPrimingFrames;
        pinfo.mPrimingFrames = 0;
        return pinfo;
    }
protected:
    size_t readSamples(void *buffer, size_t nsamples)
    {
        return (this->*m_read)(buffer, nsamples);
    }
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        return (this->*m_write)(data, length, nsamples);
    }
private:
    size_t readSamples0(void *buffer, size_t nsamples)
    {
        nsamples = std::min(nsamples, m_leading);
        uint32_t bpf = getInputDescription().mBytesPerFrame;
        memset(buffer, 0, nsamples * bpf);
        m_leading -= nsamples;
        if (!m_leading)
            m_read = &CoreAudioNoDelayEncoder::readSamples1;
        return nsamples;
    }
    size_t readSamples1(void *buffer, size_t nsamples)
    {
        return CoreAudioEncoder::readSamples(buffer, nsamples);
    }
    void writeSamples0(const void *data, size_t length, size_t nsamples)
    {
        if (++m_frames > 3)
            CoreAudioEncoder::writeSamples(data, length, nsamples);
    }
};

class CoreAudioPaddedEncoder: public CoreAudioEncoder {
    enum { LPC_ORDER = 32 };
    util::FIFO<float> m_buffer;
    std::vector<uint8_t> m_pivot;
    std::vector<uint8_t> m_frame;
    size_t m_frames;
    size_t (CoreAudioPaddedEncoder::*m_read)(void *, size_t);
    void (CoreAudioPaddedEncoder::*m_write)(const void *, size_t, size_t);
public:
    CoreAudioPaddedEncoder(AudioConverterX &converter)
        : CoreAudioEncoder(converter),
          m_frames(0),
          m_read(&CoreAudioPaddedEncoder::readSamples0)
    {
        m_buffer.set_unit(getInputDescription().mChannelsPerFrame);
        if (getOutputDescription().mFormatID == 'aac ')
            m_write = &CoreAudioPaddedEncoder::writeSamplesLC;
        else
            m_write = &CoreAudioPaddedEncoder::writeSamplesHE;
    }
    AudioFilePacketTableInfo getGaplessInfo()
    {
        AudioFilePacketTableInfo pinfo = CoreAudioEncoder::getGaplessInfo();
        if (getOutputDescription().mFormatID == 'aach') {
            pinfo.mNumberValidFrames -= 1024;
            pinfo.mRemainderFrames += 1024;
        }
        return pinfo;
    }
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
    void reverse_buffer(float *data, unsigned nsamples, unsigned nchannels)
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
    void extrapolate(float *input, size_t ilen,
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
    void extrapolate0()
    {
        unsigned nchannels = getInputDescription().mChannelsPerFrame;
        unsigned bpf = getInputDescription().mBytesPerFrame;
        unsigned fpp = getOutputDescription().mFramesPerPacket;
        std::vector<float> buf(fpp * nchannels);
        size_t n = readSamplesFull(src(), &buf[0], fpp);
        m_buffer.reserve(fpp + n);
        if (n < 2 * LPC_ORDER)
            std::memset(m_buffer.write_ptr(), 0, fpp * bpf);
        else {
            reverse_buffer(&buf[0], n, nchannels);
            extrapolate(&buf[0], n, m_buffer.write_ptr(), fpp);
            reverse_buffer(m_buffer.write_ptr(), fpp, nchannels);
            reverse_buffer(&buf[0], n, nchannels);
        }
        m_buffer.commit(fpp);
        std::copy(buf.begin(), buf.begin() + n * nchannels,
                  m_buffer.write_ptr());
        m_buffer.commit(n);
    }
    void extrapolate1()
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
    size_t readSamples0(void *buffer, size_t nsamples)
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
    size_t readSamples1(void *buffer, size_t nsamples)
    {
        size_t n = std::min(nsamples, m_buffer.count());
        if (n > 0)
            std::memcpy(buffer, m_buffer.read(n),
                        n * getInputDescription().mBytesPerFrame);
        return n;
    }
    void writeSamplesLC(const void *data, size_t length, size_t nsamples)
    {
        if (m_frame.size() && ++m_frames != 2)
            CoreAudioEncoder::writeSamples(&m_frame[0], m_frame.size(),
                                           nsamples);
        m_frame.resize(length);
        std::memcpy(&m_frame[0], data, length);
    }
    void writeSamplesHE(const void *data, size_t length, size_t nsamples)
    {
        if (++m_frames != 2)
            CoreAudioEncoder::writeSamples(data, length, nsamples);
    }
};
#endif
