#ifndef _IENCODER_H
#define _IENCODER_H

#include <vector>
#include "CoreAudioToolbox.h"
#include "CoreAudioHelper.h"

struct IEncoder {
    virtual ~IEncoder() {}
    virtual void setSource(const x::shared_ptr<ISource> &source) = 0;
    virtual void setSink(const x::shared_ptr<ISink> &sink) = 0;
    virtual bool encodeChunk(UInt32 npackets) = 0;
    virtual ISource *src() = 0;
    virtual const AudioStreamBasicDescription &getInputDescription() const = 0;
    virtual const AudioStreamBasicDescription &getOutputDescription() const = 0;
};

struct IEncoderStat {
    virtual ~IEncoderStat() {}
    virtual uint64_t samplesRead() const = 0;
    virtual uint64_t samplesWritten() const = 0;
    virtual uint64_t framesWritten() const = 0;
    virtual double currentBitrate() const = 0;
    virtual double overallBitrate() const = 0;
};

struct IEncoderOutputInfo {
    virtual ~IEncoderOutputInfo() {}
    virtual void getBasicDescription(AudioStreamBasicDescription *asbd) = 0;
    virtual void getChannelLayout(AudioChannelLayoutX *result) = 0;
    virtual void getMagicCookie(std::vector<uint8_t> *result) = 0;
};

class EncoderStat: public IEncoderStat {
    uint64_t m_samples_read;
    uint64_t m_samples_written;
    uint64_t m_frames_written;
    uint64_t m_bytes_written;
    double m_current_bitrate;
    AudioStreamBasicDescription m_desc;
public:
    EncoderStat()
        : m_samples_read(0),
          m_samples_written(0),
          m_frames_written(0),
          m_bytes_written(0)
    {}
    void setBasicDescription(const AudioStreamBasicDescription &desc)
    {
        m_desc = desc;
    }
    void updateRead(uint32_t n) { m_samples_read += n; }
    void updateWritten(uint32_t samples, uint32_t bytes)
    {
        m_frames_written += 1;
        m_samples_written += samples;
        m_bytes_written += bytes;
        m_current_bitrate = calcBitrate(bytes, samples);
    }
    uint64_t samplesRead() const { return m_samples_read; }
    uint64_t samplesWritten() const { return m_samples_written; }
    uint64_t framesWritten() const { return m_frames_written; }
    double currentBitrate() const { return m_current_bitrate; }
    double overallBitrate() const
    {
        return calcBitrate(m_bytes_written, m_samples_written);
    }
    double calcBitrate(uint64_t bytes, uint64_t samples) const
    {
        return samples ? (bytes * m_desc.mSampleRate * 8) / (1000.0 * samples)
                       : 0.0;
    }
};

#endif
