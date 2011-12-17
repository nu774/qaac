#ifndef _CHANMAP_H
#define _CHANMAP_H

#include "CoreAudioTypes.h"
#include "iointer.h"

namespace chanmap {
    uint32_t GetChannelMask(const std::vector<uint32_t>& chanmap);
    void GetChannelsFromBitmap(uint32_t bitmap, std::vector<uint32_t> *result,
			       uint32_t limit=UINT32_MAX);
    void GetChannelsFromAudioChannelLayout(const AudioChannelLayout *layout,
					   std::vector<uint32_t> *result);
    void GetChannelMappingToUSBOrder(const std::vector<uint32_t> &channels,
				     std::vector<uint32_t> *result);
    uint32_t GetDefaultChannelMask(uint32_t nchannels);
    uint32_t GetLayoutTag(uint32_t chanmask);

    /*
     * Workaround for CoreAudioToolbox >= 7.9.4.0 bug.
     * returns new layout tag and original -> AAC channel transform map.
     */
    uint32_t GetAACLayoutTag(const AudioChannelLayout *layout);
    void GetAACChannelMap(const AudioChannelLayout *layout,
	    size_t nchannels, std::vector<uint32_t> *result);
}

class ChannelMapper: public DelegatingSource {
    std::vector<uint32_t> m_chanmap;
    std::vector<uint32_t> m_layout;
public:
    ChannelMapper(const x::shared_ptr<ISource> &source,
		  const std::vector<uint32_t> &chanmap, uint32_t bitmap=0);
    const std::vector<uint32_t> *getChannels() const
    {
	return m_layout.size() ? &m_layout : 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
};

#endif
