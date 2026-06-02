#ifndef METADATA_H
#define METADATA_H

#include <iterator>
#include "mp4v2wrapper.h"
#include "misc.h"
#include "IInputStream.h"

namespace Tag {
    const uint32_t kTitle = FOURCC('\xa9','n','a','m');
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
}

namespace TextBasedTag {
    std::string normalizeTagName(const char *name);
    std::map<std::string, std::string>
        normalizeTags(const std::map<std::string, std::string> &src);
}

namespace ID3 {
    std::map<std::string, std::string> fetchAiffID3Tags(std::shared_ptr<IInputStream> stream);
    std::map<std::string, std::string> fetchMPEGID3Tags(std::shared_ptr<IInputStream> stream);
}

namespace M4A {
    enum
    {
        ITMF_TYPE_IMPLICIT  = 0,
        ITMF_TYPE_UTF8      = 1,
        ITMF_TYPE_UTF16     = 2,
        ITMF_TYPE_SJIS      = 3,
        ITMF_TYPE_HTML      = 6,
        ITMF_TYPE_XML       = 7,
        ITMF_TYPE_UUID      = 8,
        ITMF_TYPE_ISRC      = 9,
        ITMF_TYPE_MI3P      = 10,
        ITMF_TYPE_GIF       = 12,
        ITMF_TYPE_JPEG      = 13,
        ITMF_TYPE_PNG       = 14,
        ITMF_TYPE_URL       = 15,
        ITMF_TYPE_DURATION  = 16,
        ITMF_TYPE_DATETIME  = 17,
        ITMF_TYPE_GENRES    = 18,
        ITMF_TYPE_INTEGER   = 21,
        ITMF_TYPE_RIAA_PA   = 24,
        ITMF_TYPE_UPC       = 25,
        ITMF_TYPE_BMP       = 27,

        ITMF_TYPE_UNDEFINED = 255
    };

    struct ITMFItem {
        uint32_t code;
        std::string mean;
        std::string name;
        uint8_t type;
        std::string value;

        ITMFItem() : code(0), type(0) {}
    };
    const char *getTagNameFromFourCC(uint32_t fcc);

    void convertToM4ATags(const std::map<std::string, std::string> &src,
                          std::map<uint32_t, std::string> *shortTags,
                          std::map<std::string, std::string> *longTags);

    std::map<std::string, std::string> fetchTags(MP4FileX &file);

    int getImageFileType(const void *data, size_t size);

    std::vector<ITMFItem> parseUdtaMeta(const void *udta, size_t len);
    std::vector<misc::chapter_t> parseUdtaChpl(const void *udta, size_t len);
    
    std::map<std::string, std::string> convertToStringTags(const std::vector<ITMFItem> &tags);
    std::vector<ITMFItem> convertToM4aTags(const std::map<std::string, std::string> tags);
    std::vector<uint8_t> serializeUdtaMeta(const std::vector<ITMFItem> &items);
    std::vector<uint8_t> serializeUdtaChpl(const std::vector<misc::chapter_t>& items);
}

namespace CAF {
    std::map<std::string, std::string>
        fetchTags(const std::vector<uint8_t> &info);
    std::map<std::string, std::string> fetchTags(std::shared_ptr<IInputStream> stream);
}

const char * const iTunSMPB_template = " 00000000 %08X %08X %08X%08X "
"00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000";

#endif
