#ifndef _COMPOSITE_H
#define _COMPOSITE_H

#include "iointer.h"
#include "itunetags.h"

class CompositeSource: public ISeekableSource, public ITagParser {
    typedef std::shared_ptr<ISeekableSource> source_t;
    int32_t m_cur_file;
    int64_t m_position;
    uint64_t m_length;
    std::vector<source_t> m_sources;
    std::map<uint32_t, std::wstring> m_tags;
    std::vector<chapters::entry_t> m_chapters;
    AudioStreamBasicDescription m_asbd;
public:
    CompositeSource() : m_cur_file(0), m_position(0), m_length(0) {}

    const std::vector<uint32_t> *getChannels() const
    {
        return first()->getChannels();
    }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    uint64_t length() const { return m_length; }
    int64_t getPosition() { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples);
    bool isSeekable()
    {
        for (size_t i = 0; i < m_sources.size(); ++i)
            if (!m_sources[i]->isSeekable())
                return false;
        return true;
    }
    void seekTo(int64_t pos);

    const std::map<uint32_t, std::wstring> &getTags() const
    {
        if (m_tags.size()) return m_tags;
        std::shared_ptr<ISource> src = first();
        ITagParser *tp = dynamic_cast<ITagParser*>(src.get());
        return tp ? tp->getTags() : m_tags;
    }
    const std::vector<chapters::entry_t> *getChapters() const
    {
        return m_chapters.size() ? &m_chapters : 0;
    }
    void setTags(const std::map<uint32_t, std::wstring> &tags)
    {
        m_tags = tags;
    }
    void setChapters(const std::vector<chapters::entry_t> &x)
    {
        m_chapters = x;
    }

    void addSource(const std::shared_ptr<ISeekableSource> &src);
    void addSourceWithChapter(const std::shared_ptr<ISeekableSource> &src,
                              const std::wstring &title);
    size_t count() const { return m_sources.size(); }
private:
    std::shared_ptr<ISeekableSource> first() const { return m_sources[0]; }
    void addChapter(std::wstring title, double length)
    {
        m_chapters.push_back(std::make_pair(title, length));
    }
    void fetchAlbumTags(ITagParser *parser);
};

#endif
