#ifndef ASCUTIL_H
#define ASCUTIL_H

#include <stdint.h>
#include <vector>
#include "catypes.h"

namespace ascutil {
    ca::AudioStreamBasicDescription
        buildASBDForPCM(double sample_rate, unsigned channels,
                        unsigned bits, unsigned type_flags,
                        unsigned alignment=0);

    ca::AudioStreamBasicDescription
        buildASBDForPCM2(double sample_rate, unsigned channels,
                         unsigned valid_bits, unsigned pack_bits,
                         unsigned type_flags,
                         unsigned alignment=ca::kAudioFormatFlagIsAlignedHigh);

    std::vector<uint8_t>
        parseMagicCookieAAC(const std::vector<uint8_t> &cookie);

    void replaceASCInMagicCookie(std::vector<uint8_t> *cookie,
                                 const std::vector<uint8_t> &data);

    void parseASC(const std::vector<uint8_t> &asc,
                  ca::AudioStreamBasicDescription *asbd,
                  std::vector<uint32_t> *channels);

    void insertPCEToASC(std::vector<uint8_t> *asc, uint32_t layoutTag);

    bool fixupAACChannelConfigForDecode(std::vector<uint8_t> *asc);
}

#endif
