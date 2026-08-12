#ifndef MMTISOBMFFSINKBASE_H
#define MMTISOBMFFSINKBASE_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <mmtisobmff/writer/writer.h>
#include <mmtisobmff/writer/trackwriter.h>
#include "ISink.h"

class MMTISOBMFFSinkBase: public ISink, public ITagStore, public IFinishWriteSink, public IArtworkWriter, public IChapterWriter {
public:
    enum {
        MODE_ITUNSMPB = 1,
        MODE_EDTS = 2,
        MODE_BOTH = 3,
    };
    void writeSamples(const void *data, size_t length, size_t nsamples) override;
    void finishWrite(const ca::AudioFilePacketTableInfo &info) override;
    void setTag(const std::string &key, const std::string &value) override
    {
        m_tags[key] = value;
    }
    void addArtwork(const std::vector<char> &data) override
    {
        m_artworks.push_back(data);
    }
    void setChapters(const std::vector<misc::chapter_t> &chapters) override
    {
        m_chapters = chapters;
    }
protected:
    MMTISOBMFFSinkBase() = default;

    mmt::isobmff::SMovieConfig m_movieConfig;
    uint32_t m_mediaTimescale = 0;
    std::unique_ptr<mmt::isobmff::CIsobmffFileWriter> m_movieWriter;
    std::unique_ptr<mmt::isobmff::CTrackWriter> m_trackWriter;
    std::map<std::string, std::string> m_tags;
    std::vector<std::vector<char>> m_artworks;
    std::vector<misc::chapter_t> m_chapters;
    uint32_t m_gaplessMode = 0;
    uint64_t m_totalDuration = 0;
    uint32_t m_sampleDurationDivisor = 1;
private:
    MMTISOBMFFSinkBase(const MMTISOBMFFSinkBase &);
    MMTISOBMFFSinkBase& operator=(const MMTISOBMFFSinkBase &);
};

#endif
