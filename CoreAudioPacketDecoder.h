#ifndef COREAUDIOPACKETDECODER_H
#define COREAUDIOPACKETDECODER_H

#include "PacketDecoder.h"
#include "AudioConverterX.h"

class CoreAudioPacketDecoder: public IPacketDecoder {
    AudioConverterX m_converter;
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    std::vector<uint8_t> m_magic_cookie;
    std::vector<uint8_t> m_packet_buffer;
    IPacketFeeder *m_feeder;
    AudioStreamPacketDescription m_aspd;
public:
    CoreAudioPacketDecoder(IPacketFeeder *feeder,
                           const AudioStreamBasicDescription &asbd);
    void reset()
    {
        AudioConverterReset(m_converter);
    }
    void setMagicCookie(const std::vector<uint8_t> &cookie)
    {
        m_magic_cookie = cookie;
        m_converter.setDecompressionMagicCookie(cookie);
    }
    const AudioStreamBasicDescription &getSampleFormat()
    {
        return m_oasbd;
    }
    size_t decode(void *data, size_t nsamples);
private:
    static OSStatus staticInputDataProc(
            AudioConverterRef inAudioConverter,
            UInt32 *ioNumberDataPackets,
            AudioBufferList *ioData,
            AudioStreamPacketDescription **outDataPacketDescription,
            void *inUserData)
    {
        CoreAudioPacketDecoder* self =
            reinterpret_cast<CoreAudioPacketDecoder*>(inUserData);
        return self->inputDataProc(ioNumberDataPackets, ioData,
                                   outDataPacketDescription);
    }
    long inputDataProc(UInt32 *npackets, AudioBufferList *abl,
                       AudioStreamPacketDescription **aspd);
};

#endif
