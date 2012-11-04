#ifndef _COMPOSITE_H
#define _COMPOSITE_H

#include "iointer.h"
#include "itunetags.h"

class CompositeSource: public ISource, public ITagParser {
    typedef std::shared_ptr<ISource> source_t;
    std::vector<source_t> m_sources;
    AudioStreamBasicDescription m_asbd;
    size_t m_curpos;
    std::map<uint32_t, std::wstring> m_tags;
    std::vector<chapters::entry_t> m_chapters;
    uint64_t m_samples_read;
public:
    CompositeSource() : m_curpos(0), m_samples_read(0) {}
    size_t count() const { return m_sources.size(); }
    std::shared_ptr<ISource> first() const { return m_sources[0]; }
    const std::vector<uint32_t> *getChannels() const
    {
	return first()->getChannels();
    }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    const std::map<uint32_t, std::wstring> &getTags() const
    {
	if (m_tags.size()) return m_tags;
	std::shared_ptr<ISource> src = first();
	ITagParser *tp = dynamic_cast<ITagParser*>(src.get());
	return tp ? tp->getTags() : m_tags;
    }
    const std::vector<chapters::entry_t> *getChapters() const
    {
	if (m_chapters.size())
	    return &m_chapters;
	else
	    return 0;
    }
    void setTags(const std::map<uint32_t, std::wstring> &tags)
    {
	m_tags = tags;
    }
    void setChapters(const std::vector<chapters::entry_t> &x)
    {
	m_chapters = x;
    }
    void addChapter(std::wstring title, double length)
    {
	m_chapters.push_back(std::make_pair(title, length));
    }
    void addSource(const std::shared_ptr<ISource> &src)
    {
	if (!count())
	    m_asbd = src->getSampleFormat();
	else if (std::memcmp(&m_asbd, &src->getSampleFormat(), sizeof m_asbd))
	    throw std::runtime_error(
		    "CompositeSource: can't compose different sample format");
	m_sources.push_back(src);
    }
    void addSourceWithChapter(const std::shared_ptr<ISource> &src)
    {
	addSource(src);
	ITagParser *parser = dynamic_cast<ITagParser*>(src.get());
	if (parser) {
	    const std::map<uint32_t, std::wstring> &tags = parser->getTags();
	    std::map<uint32_t, std::wstring>::const_iterator tagit;
	    if (count() == 1) {
		for (tagit = tags.begin(); tagit != tags.end(); ++tagit) {
		    if (Tag::isAlbumTag(tagit->first))
			m_tags[tagit->first] = tagit->second;
		    if (tagit->first == Tag::kAlbumArtist)
			m_tags[Tag::kArtist] = tagit->second;
		    else if (tagit->first == Tag::kArtist) {
			std::map<uint32_t, std::wstring>::const_iterator it
			    = m_tags.find(Tag::kArtist);
			if (it == m_tags.end())
			    m_tags[Tag::kArtist] = tagit->second;
		    }
		}
	    }
	    const std::vector<chapters::entry_t> *chaps;
	    chaps = parser->getChapters();
	    if (chaps) {
		for (size_t i = 0; i < chaps->size(); ++i)
		    m_chapters.push_back(chaps->at(i));
		return;
	    }
	    tagit = tags.find(Tag::kTitle);
	    if (tagit != tags.end()) {
		addChapter(tagit->second, src->length() / m_asbd.mSampleRate);
		return;
	    }
	}
	addChapter(L"", src->length() / m_asbd.mSampleRate);
    }
    uint64_t length() const
    {
	uint64_t len = 0;
	for (size_t i = 0; i < m_sources.size(); ++i)
	    len += m_sources[i]->length();
	return len;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	if (m_curpos == m_sources.size())
	    return 0;
	size_t rc = m_sources[m_curpos]->readSamples(buffer, nsamples);
	m_samples_read += rc;
	if (rc == nsamples)
	    return rc;
	if (rc == 0) {
	    ++m_curpos;
	    return readSamples(buffer, nsamples);
	}
	return rc + readSamples(
	    reinterpret_cast<char*>(buffer) + rc * m_asbd.mBytesPerFrame,
	    nsamples - rc);
    }
    void setRange(int64_t start=0, int64_t length=-1)
    {
	throw std::runtime_error("CompositeSource::setRange: not implemented");
    }
    uint64_t getSamplesRead() const { return m_samples_read; }
};

#endif
