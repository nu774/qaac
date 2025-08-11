#ifndef PACKETDECODER_H
#define PACKETDECODER_H

#include <cstdint>
#include <vector>
#include "CoreAudioToolbox.h"

struct IPacketDecoder {
    virtual ~IPacketDecoder() {}
    virtual void reset() = 0;
    virtual const AudioStreamBasicDescription &getSampleFormat() = 0;
    virtual void setMagicCookie(const std::vector<uint8_t> &cookie) = 0;
    virtual size_t decode(const std::vector<uint8_t> &packet, std::vector<uint8_t> *samples) = 0;
};

#endif
