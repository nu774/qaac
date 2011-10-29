#ifndef SCAudioEncoder_H
#define SCAudioEncoder_H

#include "iointer.h"
#include "stdaudio.h"
#include "iencoder.h"

class SCAudioEncoder: public IEncoder, public IEncoderStat {
    StdAudioComponentX m_scaudio;
    x::shared_ptr<ISource> m_src;
    x::shared_ptr<ISink> m_sink;
    x::shared_ptr<AudioBufferList> m_output_abl;
    std::vector<uint8_t> m_input_buffer, m_output_buffer;
    std::vector<AudioStreamPacketDescription> m_packet_desc;
    AudioStreamBasicDescription m_input_desc, m_output_desc;
    EncoderStat m_stat;
public:
    SCAudioEncoder(StdAudioComponentX &scaudio);
    void setSource(const x::shared_ptr<ISource> &source) { m_src = source; }
    void setSink(const x::shared_ptr<ISink> &sink) { m_sink = sink; }
    bool encodeChunk(UInt32 npackets);
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
    static ComponentResult staticInputDataProc(
	    ComponentInstance ci,
	    UInt32 *ioNumberDataPackets,
	    AudioBufferList *ioData,
	    AudioStreamPacketDescription **outDataPacketDescription,
	    void *inRefCon)
    {
	SCAudioEncoder* self = reinterpret_cast<SCAudioEncoder*>(inRefCon);
	return self->inputDataProc(ioNumberDataPackets, ioData);
    }
    long inputDataProc(UInt32 *npackets, AudioBufferList *abl);
    void prepareInputBuffer(AudioBufferList *abl, size_t npackets);
    void prepareOutputBuffer(size_t npackets);
};

#endif
