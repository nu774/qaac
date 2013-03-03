#ifndef ITUNES_TAGS_H
#define ITUNES_TAGS_H

#include <iterator>
#include "iointer.h"
#include "mp4v2wrapper.h"
#include "cautil.h"
#include "CoreAudio/AudioFile.h"

namespace Tag {
    const uint32_t kTitle = FOURCC('\xa9','n','a','m');
    const uint32_t kSubTitle = FOURCC('\xa9','s','t','3');
    const uint32_t kArtist = FOURCC('\xa9','A','R','T');
    const uint32_t kAlbumArtist = 'aART';
    const uint32_t kAlbum = FOURCC('\xa9','a','l','b');
    const uint32_t kGrouping = FOURCC('\xa9','g','r','p');
    const uint32_t kComposer = FOURCC('\xa9','w','r','t');
    const uint32_t kComment = FOURCC('\xa9','c','m','t');
    const uint32_t kGenre = FOURCC('\xa9','g','e','n');
    const uint32_t kGenreID3 = 'gnre';
    const uint32_t kDate = FOURCC('\xa9','d','a','y');
    const uint32_t kTrack = 'trkn';
    const uint32_t kDisk = 'disk';
    const uint32_t kTempo = 'tmpo';
    const uint32_t kDescription = 'desc';
    const uint32_t kLongDescription = 'ldes';
    const uint32_t kLyrics = FOURCC('\xa9','l','y','r');
    const uint32_t kCopyright = 'cprt';
    const uint32_t kCompilation = 'cpil';
    const uint32_t kTool = FOURCC('\xa9','t','o','o');
    const uint32_t kArtwork = 'covr';

    const uint32_t kTvSeason = 'tvsn';
    const uint32_t kTvEpisode = 'tves';
    const uint32_t kPodcast = 'pcst';
    const uint32_t kHDVideo = 'hdvd';
    const uint32_t kMediaType = 'stik';
    const uint32_t kContentRating = 'rtng';
    const uint32_t kGapless = 'pgap';
    const uint32_t kiTunesAccountType = 'akID';
    const uint32_t kiTunesCountry = 'sfID';
    const uint32_t kcontentID = 'cnID';
    const uint32_t kartistID = 'atID';
    const uint32_t kplaylistID = 'plID';
    const uint32_t kgenreID = 'geID';
    const uint32_t kcomposerID = 'cmID';

    struct NameIDMap {
        const char *name;
        uint32_t id;
    };

    inline bool isAlbumTag(uint32_t tag)
    {
        if (tag == kAlbumArtist || tag == kAlbum || tag == kGenre ||
            tag == kGenreID3 || tag == kDate || tag == kDisk ||
            tag == kCopyright || tag == kCompilation || tag == kTool ||
            tag == kArtwork)
            return true;
        else
            return false;
    }
}

namespace ID3 {
    void fetchAiffID3Tags(int fd, std::map<uint32_t, std::wstring> *result);
    void fetchMPEGID3Tags(int fd, std::map<uint32_t, std::wstring> *result);
}

namespace Vorbis {
    uint32_t GetIDFromTagName(const char *name);
    void ConvertToItunesTags(
            const std::map<std::string, std::string> &vorbisComments,
            std::map<uint32_t, std::wstring> *itunesTags);
}

namespace mp4a {
    void fetchTags(MP4FileX &file,
                   std::map<uint32_t, std::wstring> *shortTags,
                   std::map<std::string, std::wstring> *longTags=0);
}

const wchar_t * const iTunSMPB_template = L" 00000000 %08X %08X %08X%08X "
L"00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000";

class TagEditor {
    std::map<uint32_t, std::wstring> m_tags;
    std::map<std::string, std::wstring> m_long_tags;
    std::vector<chapters::entry_t> m_chapters;
    std::vector<std::wstring> m_artworks;
    int m_encoder_delay;
    int m_padding;
    int m_gapless_mode;
    uint32_t m_artwork_size;
    uint64_t m_nsamples;
public:
    enum {
        MODE_ITUNSMPB = 1,
        MODE_EDTS = 2,
        MODE_BOTH = 3,
    };
    TagEditor()
        : m_encoder_delay(0),
          m_padding(0),
          m_gapless_mode(MODE_ITUNSMPB),
          m_artwork_size(0),
          m_nsamples(0)
    {}
    void setGaplessInfo(const AudioFilePacketTableInfo &info)
    {
        m_encoder_delay = info.mPrimingFrames;
        m_padding = info.mRemainderFrames;
        m_nsamples = info.mNumberValidFrames;
    }
    void setGaplessMode(int mode)
    {
        m_gapless_mode = mode;
    }
    void setTag(uint32_t key, const std::wstring &value)
    {
        m_tags[key] = value;
    }
    void setTag(const std::map<uint32_t, std::wstring> &tags)
    {
        std::map<uint32_t, std::wstring>::const_iterator ii;
        for (ii = tags.begin(); ii != tags.end(); ++ii)
            m_tags[ii->first] = ii->second;
    }
    void setLongTag(const std::string &key, const std::wstring &value)
    {
        m_long_tags[key] = value;
    }
    void setLongTag(const std::map<std::string, std::wstring> &tags)
    {
        std::map<std::string, std::wstring>::const_iterator ii;
        for (ii = tags.begin(); ii != tags.end(); ++ii)
            m_long_tags[ii->first] = ii->second;
    }
    void setChapters(const std::vector<chapters::entry_t> &chapters)
    {
        m_chapters = chapters;
    }
    void addArtwork(const wchar_t *filename)
    {
        m_artworks.push_back(filename);
    }
    void setArtworkSize(uint32_t size)
    {
        m_artwork_size = size;
    }
    void save(MP4FileX &mp4file);
    void saveArtworks(MP4FileX &mp4file);
};

#endif
