#ifndef LPCMPACKETDECODER_H
#define LPCMPACKETDECODER_H

#include "PacketDecoder.h"

class LPCMPacketDecoder: public IPacketDecoder {
    ca::AudioStreamBasicDescription m_iasbd;
    ca::AudioStreamBasicDescription m_oasbd;
    std::vector<uint8_t> m_pivot;
public:
    LPCMPacketDecoder();
    ~LPCMPacketDecoder() {}
    void reset() {}
    const ca::AudioStreamBasicDescription &getInputFormat() { return m_iasbd; }
    const ca::AudioStreamBasicDescription &getSampleFormat() { return m_oasbd; }
    void setMagicCookie(const std::vector<uint8_t> &cookie);
    size_t decode(const std::vector<uint8_t> &packet, std::vector<uint8_t> *samples);
};

#endif

