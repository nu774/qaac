#ifndef _CHANMAP_H
#define _CHANMAP_H

#include <stdint.h>
#include <string>
#include <vector>
#include "CoreAudio/CoreAudioTypes.h"

namespace chanmap {
    const int kAudioChannelLayoutTag_AAC_7_1_Rear = 0x01000008;
    std::string getChannelNames(const std::vector<uint32_t> &channels);
    uint32_t getChannelMask(const std::vector<uint32_t>& chanmap);
    void getChannels(uint32_t bitmap, std::vector<uint32_t> *result,
                     uint32_t limit=~0U);
    void getChannels(const AudioChannelLayout *layout,
                     std::vector<uint32_t> *result);
    void convertFromAppleLayout(std::vector<uint32_t> *channels);
    void getMappingToUSBOrder(const std::vector<uint32_t> &channels,
                              std::vector<uint32_t> *result);
    uint32_t defaultChannelMask(uint32_t nchannels);
    uint32_t AACLayoutFromBitmap(uint32_t bitmap);
    void getMappingToAAC(uint32_t bitmap, std::vector<uint32_t> *result);
}

#endif
