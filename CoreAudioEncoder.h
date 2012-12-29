#ifndef CoreAudioEncoder_H
#define CoreAudioEncoder_H

#include "CoreAudioToolbox.h"
#include "AudioConverterX.h"
#include "iointer.h"
#include "iencoder.h"

class CoreAudioEncoder: public IEncoder, public IEncoderStat {
    AudioConverterX m_converter;
    bool m_requires_packet_desc;
    bool m_variable_packet_len;
    std::shared_ptr<ISource> m_src;
    std::shared_ptr<ISink> m_sink;
    std::shared_ptr<AudioBufferList> m_output_abl;
    std::vector<uint8_t> m_input_buffer, m_output_buffer;
    std::vector<AudioStreamPacketDescription> m_packet_desc;
    AudioStreamBasicDescription m_input_desc, m_output_desc;
    EncoderStat m_stat;
public:
    CoreAudioEncoder(AudioConverterX &converter);
    void setSource(const std::shared_ptr<ISource> &source) { m_src = source; }
    void setSink(const std::shared_ptr<ISink> &sink) { m_sink = sink; }
    uint32_t encodeChunk(UInt32 npackets);
    AudioFilePacketTableInfo getGaplessInfo();
    AudioConverterX getConverter() { return m_converter; }
    ISource *src() { return m_src.get(); }
    const AudioStreamBasicDescription &getInputDescription() const
    {
        return m_input_desc;
    }
    const AudioStreamBasicDescription &getOutputDescription() const
    {
        return m_output_desc;
    }
    uint64_t samplesRead() const { return m_stat.samplesRead(); }
    uint64_t samplesWritten() const { return m_stat.samplesWritten(); }
    uint64_t framesWritten() const { return m_stat.framesWritten(); }
    double currentBitrate() const { return m_stat.currentBitrate(); }
    double overallBitrate() const { return m_stat.overallBitrate(); }
private:
    static OSStatus staticInputDataProc(
            AudioConverterRef inAudioConverter,
            UInt32 *ioNumberDataPackets,
            AudioBufferList *ioData,
            AudioStreamPacketDescription **outDataPacketDescription,
            void *inUserData)
    {
        CoreAudioEncoder* self =
            reinterpret_cast<CoreAudioEncoder*>(inUserData);
        return self->inputDataProc(ioNumberDataPackets, ioData);
    }
    long inputDataProc(UInt32 *npackets, AudioBufferList *abl);
    void prepareInputBuffer(AudioBufferList *abl, size_t npackets);
    void prepareOutputBuffer(size_t npackets);
};

#endif
