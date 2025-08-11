#ifndef LPCMPACKETDECODER_H
#define LPCMPACKETDECODER_H

#include "CoreAudioTypes.h"
#include "PacketDecoder.h"

class LPCMPacketDecoder: public IPacketDecoder {
    AudioStreamBasicDescription m_iasbd;
    AudioStreamBasicDescription m_oasbd;
    std::vector<uint8_t> m_pivot;
public:
    LPCMPacketDecoder();
    ~LPCMPacketDecoder() {}
    void reset() {}
    const AudioStreamBasicDescription &getInputFormat() { return m_iasbd; }
    const AudioStreamBasicDescription &getSampleFormat() { return m_oasbd; }
    void setMagicCookie(const std::vector<uint8_t> &cookie);
    size_t decode(const std::vector<uint8_t> &packet, std::vector<uint8_t> *samples);
};

#endif

