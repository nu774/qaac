#ifndef ALACPACKETDECODER_H
#define ALACPACKETDECODER_H

#include <memory>
#include <ALACDecoder.h>
#include "PacketDecoder.h"
#include "util.h"

class ALACPacketDecoder: public IPacketDecoder {
    IPacketFeeder *m_feeder;
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    std::shared_ptr<ALACDecoder> m_decoder;
    std::vector<uint8_t> m_packet_buffer, m_raw_decode_buffer;
    util::FIFO<int32_t> m_decode_buffer;
public:
    ALACPacketDecoder(IPacketFeeder *feeder,
                      const AudioStreamBasicDescription &asbd);
    void reset() {}
    const AudioStreamBasicDescription &getSampleFormat()
    {
        return m_oasbd;
    }
    void setMagicCookie(const std::vector<uint8_t> &cookie)
    {
        m_decoder->Init(const_cast<uint8_t*>(cookie.data()), cookie.size());
    }
    size_t decode(void *data, size_t nsamples);
};

#endif
