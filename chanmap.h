#ifndef _CHANMAP_H
#define _CHANMAP_H

#include "iointer.h"

uint32_t
GetChannelLayoutTagFromChannelMap(const std::vector<uint32_t>& chanmap);

class ChannelMapper: public ISource, public ITagParser {
    ISource *m_src;
    std::vector<uint32_t> m_chanmap;
    std::map<uint32_t, std::wstring> m_emptyTags;
public:
    ChannelMapper(ISource *source, const std::vector<uint32_t> &chanmap)
	: m_src(source)
    {
	for (size_t i = 0; i < chanmap.size(); ++i)
	    m_chanmap.push_back(chanmap[i] - 1);
    }
    uint64_t length() const { return m_src->length(); }
    const SampleFormat &getSampleFormat() const
    {
	return m_src->getSampleFormat(); 
    }
    const std::vector<uint32_t> *getChannelMap() const
    {
	return m_src->getChannelMap();
    }
    const std::map<uint32_t, std::wstring> &getTags() const
    {
	ITagParser *parser = dynamic_cast<ITagParser*>(m_src);
	if (!parser)
	    return m_emptyTags;
	else
	    return parser->getTags();
    }
    const std::vector<std::pair<std::wstring, int64_t> > * getChapters() const
    {
	ITagParser *parser = dynamic_cast<ITagParser*>(m_src);
	if (!parser)
	    return 0;
	else
	    return parser->getChapters();
    }
    size_t readSamples(void *buffer, size_t nsamples);
};

#endif
