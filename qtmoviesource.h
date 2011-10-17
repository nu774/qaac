#ifndef _QTMOVIESOURCE_H
#define _QTMOVIESOURCE_H

#include <vector>
#include "qtmoviehelper.h"
#include "movieaudio.h"
#include "iointer.h"

class QTMovieSource:
    public ISource, public ITagParser, public PartialSource<QTMovieSource>
{
    bool m_extraction_complete;
    std::vector<uint32_t> m_chanmap;
    std::vector<uint32_t> m_channel_conversion_map;
    std::map<uint32_t, std::wstring> m_tags;
    MovieAudioExtractionX m_session;
    SampleFormat m_format;
    AudioStreamBasicDescription m_description;
public:
    explicit QTMovieSource(const std::wstring &path);
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const
    {
	return m_chanmap.size() ? &m_chanmap : 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const { return 0; }
    void setRenderQuality(uint32_t value)
    {
	m_session.setRenderQuality(value);
    }
private:
    void fetchTags(Movie movie);
};

#endif
