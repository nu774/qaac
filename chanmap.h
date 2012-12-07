#ifndef _CHANMAP_H
#define _CHANMAP_H

#include <stdint.h>
#include "CoreAudio/CoreAudioTypes.h"
#include "iointer.h"

namespace chanmap {
    std::string getChannelNames(const std::vector<uint32_t> &channels);
    uint32_t getChannelMask(const std::vector<uint32_t>& chanmap);
    void getChannels(uint32_t bitmap, std::vector<uint32_t> *result,
                     uint32_t limit=~0U);
    void getChannels(const AudioChannelLayout *layout,
                     std::vector<uint32_t> *result);
    void convertFromAppleLayout(const std::vector<uint32_t> &from,
                                std::vector<uint32_t> *to);
    void getMappingToUSBOrder(const std::vector<uint32_t> &channels,
                              std::vector<uint32_t> *result);
    uint32_t defaultChannelMask(uint32_t nchannels);
    uint32_t AACLayoutFromBitmap(uint32_t bitmap);
    void getMappingToAAC(uint32_t bitmap, std::vector<uint32_t> *result);
}

class ChannelMapper: public FilterBase {
    std::vector<uint32_t> m_chanmap;
    std::vector<uint32_t> m_layout;
public:
    ChannelMapper(const std::shared_ptr<ISource> &source,
                  const std::vector<uint32_t> &chanmap, uint32_t bitmap=0);
    const std::vector<uint32_t> *getChannels() const
    {
        return m_layout.size() ? &m_layout : 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
};

#endif
