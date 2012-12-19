#ifndef ITUNES_TAGS_H
#define ITUNES_TAGS_H

#include <iterator>
#include "utf8_codecvt_facet.hpp"
#include "iointer.h"
#include "iff.h"
#include "mp4v2wrapper.h"

namespace Tag {
    const uint32_t kTitle = fourcc("\xa9""nam");
    const uint32_t kSubTitle = fourcc("\xa9""st3");
    const uint32_t kArtist = fourcc("\xa9""ART");
    const uint32_t kAlbumArtist = 'aART';
    const uint32_t kAlbum = fourcc("\xa9""alb");
    const uint32_t kGrouping = fourcc("\xa9""grp");
    const uint32_t kComposer = fourcc("\xa9""wrt");
    const uint32_t kComment = fourcc("\xa9""cmt");
    const uint32_t kGenre = fourcc("\xa9""gen");
    const uint32_t kGenreID3 = fourcc("gnre");
    const uint32_t kDate = fourcc("\xa9""day");
    const uint32_t kTrack = 'trkn';
    const uint32_t kDisk = 'disk';
    const uint32_t kTempo = 'tmpo';
    const uint32_t kDescription = 'desc';
    const uint32_t kLongDescription = 'ldes';
    const uint32_t kLyrics = fourcc("\xa9""lyr");
    const uint32_t kCopyright = 'cprt';
    const uint32_t kCompilation = 'cpil';
    const uint32_t kTool = fourcc("\xa9""too");
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
    uint32_t GetIDFromTagName(const char *name);
    void fetchAiffID3Tags(const wchar_t *filename,
                          std::map<uint32_t, std::wstring> *result);
}

namespace Vorbis {
    uint32_t GetIDFromTagName(const char *name);
    const char *GetNameFromTagID(uint32_t fcc);
    void ConvertToItunesTags(
            const std::map<std::string, std::string> &vorbisComments,
            std::map<uint32_t, std::wstring> *itunesTags);
    void ConvertFromItunesTags(
            const std::map<uint32_t, std::wstring> &itunesTags,
            std::map<std::string, std::string> *vorbisComments);
}

namespace mp4a {
    void fetchTags(MP4FileX &file,
                   std::map<uint32_t, std::wstring> *shortTags,
                   std::map<std::string, std::wstring> *longTags=0);
}

const char * const iTunSMPB_template = " 00000000 %08X %08X %08X%08X "
"00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000";

class TagEditor {
    std::map<uint32_t, std::wstring> m_tags;
    std::map<std::string, std::wstring> m_long_tags;
    std::vector<std::pair<std::wstring, int64_t> > m_chapters;
    std::vector<std::wstring> m_artworks;
    int m_encoder_delay;
    int m_padding;
    uint32_t m_artwork_size;
    uint64_t m_nsamples;
public:
    TagEditor()
        : m_encoder_delay(0),
          m_padding(0),
          m_artwork_size(0),
          m_nsamples(0)
    {}
    void setGaplessInfo(const GaplessInfo &info)
    {
        m_encoder_delay = info.delay;
        m_padding = info.padding;
        m_nsamples = info.samples;
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
    void setChapters(const std::vector<std::pair<std::wstring, int64_t> >
            &chapters)
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
