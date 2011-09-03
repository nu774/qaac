#ifndef _CHANMAP_H
#define _CHANMAP_H

#include "iointer.h"

uint32_t
GetChannelLayoutTagFromChannelMap(const std::vector<uint32_t>& chanmap);

class ChannelMapper: public DelegatingSource {
    std::vector<uint32_t> m_chanmap;
public:
    ChannelMapper(ISource *source, const std::vector<uint32_t> &chanmap)
	: DelegatingSource(source)
    {
	for (size_t i = 0; i < chanmap.size(); ++i)
	    m_chanmap.push_back(chanmap[i] - 1);
    }
    size_t readSamples(void *buffer, size_t nsamples);
};

#endif
