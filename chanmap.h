#ifndef _CHANMAP_H
#define _CHANMAP_H

#include <stdint.h>
#include <string>
#include <vector>
#include "CoreAudio/CoreAudioTypes.h"

namespace chanmap {
    std::string getChannelNames(const std::vector<uint32_t> &channels);
    uint32_t getChannelMask(const std::vector<uint32_t>& chanmap);
    std::vector<uint32_t> getChannels(uint32_t bitmap, uint32_t limit=~0U);
    std::vector<uint32_t> getChannels(const AudioChannelLayout *layout);
    std::vector<uint32_t>
        convertFromAppleLayout(const std::vector<uint32_t> &channels);
    std::vector<uint32_t>
        getMappingToUSBOrder(const std::vector<uint32_t> &channels);
    uint32_t defaultChannelMask(uint32_t nchannels);
    uint32_t AACLayoutFromBitmap(uint32_t bitmap);
    std::vector<uint32_t> getMappingToAAC(uint32_t bitmap);
}

#endif
