#include "composite.h"

size_t CompositeSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_cur_file == m_sources.size())
	return 0;
    size_t rc = m_sources[m_cur_file]->readSamples(buffer, nsamples);
    if (rc > 0) {
	m_position += rc;
	return rc;
    } else {
	++m_cur_file;
	if (m_cur_file < m_sources.size())
	    m_sources[m_cur_file]->seekTo(0);
	return readSamples(buffer, nsamples);
    }
}

void CompositeSource::seekTo(int64_t pos)
{
    if (pos == m_position)
	return;
    uint64_t acc = 0;
    for (m_cur_file = 0; m_cur_file < m_sources.size(); ++m_cur_file) {
	uint64_t len = m_sources[m_cur_file]->length();
	if (acc <= pos && pos < acc + len)
	    break;
	acc += len;
    }
    if (m_cur_file == m_sources.size())
	throw std::runtime_error("Invalid seek offset");
    for (uint32_t i = m_cur_file + 1; i < m_sources.size(); ++i)
	m_sources[i]->seekTo(0);
    m_sources[m_cur_file]->seekTo(pos - acc);
    m_position = pos;
}

void CompositeSource::addSource(const std::shared_ptr<ISeekableSource> &src)
{
    if (!count())
	m_asbd = src->getSampleFormat();
    else if (std::memcmp(&m_asbd, &src->getSampleFormat(), sizeof m_asbd))
	throw std::runtime_error(
		"CompositeSource: can't compose different sample format");
    m_sources.push_back(src);
    uint64_t len = src->length();
    if (len != ~0ULL && m_length != ~0ULL)
	m_length += src->length();
    else if (len == ~0ULL)
	m_length = ~0ULL;
}

void CompositeSource::addSourceWithChapter(
			    const std::shared_ptr<ISeekableSource> &src,
			    const std::wstring &title)
{
    addSource(src);
    ITagParser *parser = dynamic_cast<ITagParser*>(src.get());
    if (parser) {
	if (count() == 1)
	    fetchAlbumTags(parser);
	const std::vector<chapters::entry_t> *chaps;
	if ((chaps = parser->getChapters())) {
	    std::copy(chaps->begin(), chaps->end(),
		      std::back_inserter(m_chapters));
	    return;
	}
	const std::map<uint32_t, std::wstring> &tags = parser->getTags();
	std::map<uint32_t, std::wstring>::const_iterator
	    tag = tags.find(Tag::kTitle);
	if (tag != tags.end()) {
	    addChapter(tag->second, src->length() / m_asbd.mSampleRate);
	    return;
	}
    }
    addChapter(title, src->length() / m_asbd.mSampleRate);
}

void CompositeSource::fetchAlbumTags(ITagParser *parser)
{
    const std::map<uint32_t, std::wstring> &tags = parser->getTags();
    std::map<uint32_t, std::wstring>::const_iterator tagit;
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

