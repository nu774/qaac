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
        throw std::runtime_error("Concatenation of multiple inputs with "
                                 "different sample format is not supported");
    m_sources.push_back(src);
    uint64_t len = src->length();
    if (len != ~0ULL && m_length != ~0ULL)
        m_length += src->length();
    else if (len == ~0ULL)
        m_length = ~0ULL;

    /*
     * Discard tags that take different values on each song.
     * This way, only album common tags should remain.
     */
    ITagParser *parser = dynamic_cast<ITagParser*>(src.get());
    if (parser) {
        const std::map<std::string, std::string> &tags = parser->getTags();
        std::map<std::string, std::string>::const_iterator s;
        bool is_empty = m_tags.empty();
        for (s = tags.begin(); s != tags.end(); ++s) {
            if (is_empty)
                m_tags[s->first] = s->second;
            else if (m_tags.find(s->first) != m_tags.end())
                m_tags.erase(s->first);
        }
    }
}

void CompositeSource::addSourceWithChapter(
                            const std::shared_ptr<ISeekableSource> &src,
                            const std::wstring &title)
{
    addSource(src);
    ITagParser *parser = dynamic_cast<ITagParser*>(src.get());
    if (parser) {
        const std::vector<chapters::entry_t> *chaps;
        if ((chaps = parser->getChapters())) {
            std::copy(chaps->begin(), chaps->end(),
                      std::back_inserter(m_chapters));
            return;
        }
        const std::map<std::string, std::string> &tags = parser->getTags();
        std::map<std::string, std::string>::const_iterator
            tag = tags.find("title");
        if (tag != tags.end()) {
            addChapter(strutil::us2w(tag->second),
                       src->length() / m_asbd.mSampleRate);
            return;
        }
    }
    addChapter(title, src->length() / m_asbd.mSampleRate);
}
