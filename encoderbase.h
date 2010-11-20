#ifndef _ENCODERBASE_H
#define _ENCODERBASE_H

#include "stdaudio.h"
#include "iointer.h"

class EncoderBase : public StdAudioComponentX {
protected:
    ISource *m_src;
    ISink *m_sink;
    std::vector<AudioStreamPacketDescription> m_packet_desc;
    std::vector<char> m_input_buffer, m_output_buffer;
    uint64_t m_samples_read, m_frames_written, m_bytes_written;
    double m_max_bitrate, m_cur_bitrate;
    AudioStreamBasicDescription m_input_desc, m_output_desc;
public:
    EncoderBase(ISource *src, uint32_t formatID);
    void setSink(ISink &sink) { m_sink = &sink; }

    uint64_t samplesRead() const { return m_samples_read; }
    uint64_t framesWritten() const { return m_frames_written; }
    uint64_t bytesWritten() const { return m_bytes_written; }
    double currentBitrate() const { return m_cur_bitrate; }
    double maxBitrate() const { return m_max_bitrate; }
    double overallBitrate() const
    {
	return calcBitrate(m_bytes_written,
		m_output_desc.mFramesPerPacket * m_frames_written);
    }
    bool encodeChunk(UInt32 nframes);
    const AudioStreamBasicDescription &getInputBasicDescription() const
    {
	return m_input_desc;
    }
    const AudioStreamBasicDescription &getOutputBasicDescription() const
    {
	return m_output_desc;
    }
    void setOutputBasicDescription(const AudioStreamBasicDescription &desc)
    {
	setBasicDescription(desc);
	m_output_desc = desc;
    }
    ISource *src() { return m_src; }
    ISink *sink() { return m_sink; }
private:
    static ComponentResult staticInputDataProc(
	    ComponentInstance ci,
	    UInt32 *ioNumberDataPackets,
	    AudioBufferList *ioData,
	    AudioStreamPacketDescription **outDataPacketDescription,
	    void *inRefCon)
    {
	EncoderBase* self = reinterpret_cast<EncoderBase*>(inRefCon);
	return self->inputDataProc(ioNumberDataPackets, ioData);
    }
    double calcBitrate(uint64_t bytes, uint64_t samples) const
    {
	return (bytes * m_output_desc.mSampleRate * 8) / (1000.0 * samples);
    }
    long inputDataProc(UInt32 *nframes, AudioBufferList *abl);
    void prepareOutputBuffer(uint32_t nframes);
    void prepareInputBuffer(AudioBufferList *abl, size_t nframe);
};

#endif
