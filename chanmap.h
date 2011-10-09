#ifndef _CHANMAP_H
#define _CHANMAP_H

#include <CoreAudioTypes.h>
#include "iointer.h"

void MapChannelLabel(AudioChannelDescription *desc, uint32_t bitmap);
uint32_t GetChannelMask(const std::vector<uint32_t>& chanmap);
uint32_t GetDefaultChannelMask(uint32_t nchannels);
uint32_t GetLayoutTag(uint32_t chanmask);

/*
 * Workaround for CoreAudioToolbox >= 7.9.4.0 bug.
 * returns new layout tag and original -> AAC channel transform map.
 */
uint32_t GetAACLayoutTag(const AudioChannelLayout *layout);
void GetAACChannelMap(const AudioChannelLayout *layout,
	size_t nchannels, std::vector<uint32_t> *result);

class ChannelMapper: public DelegatingSource {
    std::vector<uint32_t> m_chanmap;
public:
    ChannelMapper(const x::shared_ptr<ISource> &source,
	const std::vector<uint32_t> &chanmap)
	: DelegatingSource(source)
    {
	for (size_t i = 0; i < chanmap.size(); ++i)
	    m_chanmap.push_back(chanmap[i] - 1);
    }
    size_t readSamples(void *buffer, size_t nsamples);
};

#endif
