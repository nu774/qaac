#ifndef CoreAudioEncoder_H
#define CoreAudioEncoder_H

#include "CoreAudioToolbox.h"
#include "AudioConverterXX.h"
#include "IEncoder.h"

class CoreAudioEncoder: public IEncoder, public IEncoderStat {
    AudioConverterXX m_converter;
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
    CoreAudioEncoder(AudioConverterXX &converter);
    virtual ~CoreAudioEncoder() {};
    void setSource(const std::shared_ptr<ISource> &source) { m_src = source; }
    void setSink(const std::shared_ptr<ISink> &sink) { m_sink = sink; }
    uint32_t encodeChunk(UInt32 npackets);
    virtual AudioFilePacketTableInfo getGaplessInfo();
    AudioConverterXX &getConverter() { return m_converter; }
    ISource *src() { return m_src.get(); }
    const AudioStreamBasicDescription &getInputDescription() const
    {
        return m_input_desc;
    }
    const AudioStreamBasicDescription &getOutputDescription() const
    {
        return m_output_desc;
    }
    uint64_t samplesRead() const { return m_src->getPosition(); }
    uint64_t samplesWritten() const { return m_stat.samplesWritten(); }
    uint64_t framesWritten() const { return m_stat.framesWritten(); }
    double currentBitrate() const { return m_stat.currentBitrate(); }
    double overallBitrate() const { return m_stat.overallBitrate(); }
protected:
    virtual size_t readSamples(void *buffer, size_t nsamples)
    {
        return m_src->readSamples(buffer, nsamples);
    }
    virtual void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        m_sink->writeSamples(data, length, nsamples);
        m_stat.updateWritten(nsamples, length);
    }
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
