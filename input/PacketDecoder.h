#ifndef PACKETDECODER_H
#define PACKETDECODER_H

#include <cstdint>
#include <vector>
#include "CoreAudioToolbox.h"

struct IPacketFeeder {
    virtual bool feed(std::vector<uint8_t> *packet) = 0;
};

struct IPacketDecoder {
    virtual ~IPacketDecoder() {}
    virtual void reset() = 0;
    virtual const AudioStreamBasicDescription &getSampleFormat() = 0;
    virtual void setMagicCookie(const std::vector<uint8_t> &cookie) = 0;
    virtual size_t decode(void *data, size_t nsamples) = 0;
};

#endif
