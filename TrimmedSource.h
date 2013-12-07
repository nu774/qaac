#ifndef RANGEDSOURCE_H
#define RANGEDSOURCE_H

#include "iointer.h"

class TrimmedSource: public ISeekableSource, public ITagParser {
    uint64_t m_start;
    uint64_t m_duration;
    int64_t m_position;
    std::shared_ptr<ISeekableSource> m_src;
    std::map<std::string, std::string> m_emptyTags;
public:
    TrimmedSource(const std::shared_ptr<ISeekableSource> &src)
        : m_src(src), m_position(0)
    {
        setRange(0, m_src->length());
    }
    TrimmedSource(const std::shared_ptr<ISeekableSource> &src,
                 uint64_t start, uint64_t duration)
        : m_src(src), m_position(0)
    {
        setRange(start, duration);
    }

    uint64_t length() const { return m_duration; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_src->getSampleFormat();
    }
    const std::vector<uint32_t> *getChannels() const
    {
        return m_src->getChannels();
    }
    int64_t getPosition()
    {
        return m_src->getPosition() - m_start;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
        nsamples = std::min(static_cast<uint64_t>(nsamples),
                            m_duration - m_position);
        if (nsamples) {
            nsamples = m_src->readSamples(buffer, nsamples);
            m_position += nsamples;
        }
        return nsamples;
    }

    bool isSeekable() { return m_src->isSeekable(); }

    void seekTo(int64_t count)
    {
        m_src->seekTo(m_start + count);
        m_position = count;
    }

    const std::map<std::string, std::string> &getTags() const
    {
        ITagParser *parser = dynamic_cast<ITagParser*>(m_src.get());
        if (!parser)
            return m_emptyTags;
        else
            return parser->getTags();
    }
    const std::vector<chapters::entry_t> * getChapters() const
    {
        ITagParser *parser = dynamic_cast<ITagParser*>(m_src.get());
        if (!parser)
            return 0;
        else
            return parser->getChapters();
    }

    void setRange(uint64_t start, uint64_t duration)
    {
        uint64_t len = m_src->length();
        if (start > len || (duration != ~0ULL && start + duration > len))
            throw std::runtime_error("Start/end offset exceeds "
                                     "length of input file");
        m_start = start;
        if (duration != ~0ULL)
            m_duration = duration;
        else if (len != ~0ULL)
            m_duration = len - m_start;
        else
            m_duration = ~0ULL;
    }
};

#endif
