#ifndef _CHANMAP_H
#define _CHANMAP_H

#include <CoreAudioTypes.h>
#include "iointer.h"

void MapChannelLabel(AudioChannelDescription *desc, uint32_t bitmap);
void AdjustChannelLabel(AudioChannelDescription *desc, uint32_t bitmap,
	uint32_t outLayoutTag);
uint32_t GetChannelMask(const std::vector<uint32_t>& chanmap);
uint32_t GetDefaultChannelMask(uint32_t nchannels);
uint32_t GetLayoutTag(uint32_t chanmask);
uint32_t GetALACLayoutTag(uint32_t nchannels);

uint32_t GetAACReversedChannelMap(uint32_t layoutTag,
	std::vector<uint32_t> *result);
/*
 * Workaround for CoreAudioToolbox >= 7.9.4.0 bug.
 * returns new layout tag and original -> AAC channel transform map.
 */
uint32_t GetAACLayoutTag(const AudioChannelLayout *layout);
void GetAACChannelMap(const AudioChannelLayout *layout,
	size_t nchannels, std::vector<uint32_t> *result);

class ChannelMapper: public DelegatingSource {
    std::vector<uint32_t> m_chanmap;
    std::vector<uint32_t> m_layout;
public:
    ChannelMapper(const x::shared_ptr<ISource> &source,
	const std::vector<uint32_t> &chanmap, uint32_t bitmap=0)
	: DelegatingSource(source)
    {
	for (size_t i = 0; i < chanmap.size(); ++i)
	    m_chanmap.push_back(chanmap[i] - 1);
	if (bitmap) {
	    for (size_t i = 0; i < 32; ++i, bitmap >>= 1)
		if (bitmap & 1) m_layout.push_back(i + 1);
	}
    }
    const std::vector<uint32_t> *getChannelMap()
    {
	if (m_layout.size())
	    return &m_layout;
	else
	    return DelegatingSource::getChannelMap();
    }
    size_t readSamples(void *buffer, size_t nsamples);
};

#endif
