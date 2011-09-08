#ifndef _CHANMAP_H
#define _CHANMAP_H

#include "iointer.h"

uint32_t
GetChannelLayoutTagFromChannelMap(const std::vector<uint32_t>& chanmap);

/*
 * Workaround for CoreAudioToolbox >= 7.9.4.0 bug.
 * returns new layout tag and original -> AAC channel transform map.
 */
uint32_t GetAACChannelMapFromLayoutTag(
	uint32_t tag, std::vector<uint32_t> *result);

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
