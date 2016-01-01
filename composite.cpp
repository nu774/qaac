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
    m_length = (len > ~0ULL - m_length) ? ~0ULL : m_length + len;

    /*
     * Want to discard tags that take different values on each track.
     * This way, only (per-album) common tags should finally remain.
     */
    auto parser = dynamic_cast<ITagParser*>(src.get());
    if (!parser)
        return;
    auto tags = parser->getTags();
    bool is_empty = m_tags.empty();
    std::for_each(tags.begin(), tags.end(),
                  [&](const decltype(*tags.begin()) &kv) {
                      if (is_empty)
                          m_tags[kv.first] = kv.second;
                      else {
                          auto it = m_tags.find(kv.first);
                          if (it != m_tags.end() && it->second != kv.second)
                              m_tags.erase(it);
                      }
                  });
}

void CompositeSource::addSourceWithChapter(
                            const std::shared_ptr<ISeekableSource> &src,
                            const std::wstring &title)
{
    addSource(src);
    std::wstring name(title);
    auto parser = dynamic_cast<ITagParser*>(src.get());
    auto cp = dynamic_cast<IChapterParser*>(src.get());
    if (cp) {
        auto &chaps = cp->getChapters();
        if (chaps.size()) {
            std::copy(chaps.begin(), chaps.end(),
                      std::back_inserter(m_chapters));
            return;
        }
    }
    if (parser) {
        auto tags = parser->getTags();
        if (tags.find("title") != tags.end())
            name = strutil::us2w(tags["title"]);
    }
    addChapter(name, src->length() / m_asbd.mSampleRate);
}
