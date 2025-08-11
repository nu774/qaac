#ifndef ALACPACKETDECODER_H
#define ALACPACKETDECODER_H

#include <memory>
#include <ALACDecoder.h>
#include "PacketDecoder.h"

class ALACPacketDecoder: public IPacketDecoder {
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    std::shared_ptr<ALACDecoder> m_decoder;
    std::vector<uint8_t> m_raw_decode_buffer;
public:
    ALACPacketDecoder(const AudioStreamBasicDescription &asbd);
    void reset() {}
    const AudioStreamBasicDescription &getSampleFormat()
    {
        return m_oasbd;
    }
    void setMagicCookie(const std::vector<uint8_t> &cookie)
    {
        m_decoder->Init(const_cast<uint8_t*>(cookie.data()), cookie.size());
    }
    virtual size_t decode(const std::vector<uint8_t> &packet, std::vector<uint8_t> *samples);
};

#endif
