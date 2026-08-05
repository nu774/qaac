#ifndef ALACENC_H
#define ALACENC_H

#include "iencoder.h"
#include <stdint.h>
#include <ALACEncoder.h>

class ALACEncoderX: public IEncoder, public IEncoderStat {
    std::shared_ptr<ISource> m_src;
    std::shared_ptr<ISink> m_sink;
    std::shared_ptr<ALACEncoder> m_encoder;
    std::vector<uint8_t> m_input_buffer;
    std::vector<uint8_t> m_output_buffer;
    ca::AudioStreamBasicDescription m_iasbd;
    AudioFormatDescription m_iafd;
    ca::AudioStreamBasicDescription m_oasbd;
    AudioFormatDescription m_oafd;
    EncoderStat m_stat;
public:
    ALACEncoderX(const ca::AudioStreamBasicDescription &desc);
    void setFastMode(bool fast) { m_encoder->SetFastMode(fast); }
    uint32_t encodeChunk(uint32_t npackets);
    std::vector<uint8_t> getMagicCookie();
    void setSource(const std::shared_ptr<ISource> &source) { m_src = source; }
    void setSink(const std::shared_ptr<ISink> &sink) { m_sink = sink; }
    ISource *src() { return m_src.get(); }
    const ca::AudioStreamBasicDescription &getInputDescription() const
    {
        return m_iasbd;
    }
    const ca::AudioStreamBasicDescription &getOutputDescription() const
    {
        return m_oasbd;
    }
    uint64_t samplesRead() const { return m_src->getPosition(); }
    uint64_t samplesWritten() const { return m_stat.samplesWritten(); }
    uint64_t framesWritten() const { return m_stat.framesWritten(); }
    double currentBitrate() const { return m_stat.currentBitrate(); }
    double overallBitrate() const { return m_stat.overallBitrate(); }

    static bool isAvailableOutputChannelLayout(uint32_t channel_layout_tag);
};

#endif
