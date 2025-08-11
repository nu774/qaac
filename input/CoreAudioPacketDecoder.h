#ifndef COREAUDIOPACKETDECODER_H
#define COREAUDIOPACKETDECODER_H

#include "PacketDecoder.h"
#include "AudioConverterX.h"

class CoreAudioPacketDecoder: public IPacketDecoder {
    enum {
        NO_PRIMING_PACKET,
        NEED_PRIMING_PACKET,
        HAVE_PRIMING_PACKET,
    };
    AudioConverterX m_converter;
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    std::vector<uint8_t> m_magic_cookie;
    std::vector<uint8_t> m_packet_buffer;
    std::vector<uint8_t> m_priming_packet;
    AudioStreamPacketDescription m_aspd;
    int m_priming_packet_state;
public:
    CoreAudioPacketDecoder(const AudioStreamBasicDescription &asbd);
    void reset()
    {
        AudioConverterReset(m_converter);
        switch (m_iasbd.mFormatID) {
        case 'aach': case 'aacp':
            m_priming_packet_state = NEED_PRIMING_PACKET;
        }
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
    size_t decode(const std::vector<uint8_t> &packet, std::vector<uint8_t> *samples);
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
