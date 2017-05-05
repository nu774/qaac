#ifndef METADATA_H
#define METADATA_H

#include <iterator>
#include "mp4v2wrapper.h"
#include "cautil.h"
#include "CoreAudio/AudioFile.h"

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
    std::map<std::string, std::string> fetchAiffID3Tags(int fd);
    std::map<std::string, std::string> fetchMPEGID3Tags(int fd);
}

namespace M4A {
    const char *getTagNameFromFourCC(uint32_t fcc);

    void convertToM4ATags(const std::map<std::string, std::string> &src,
                          std::map<uint32_t, std::string> *shortTags,
                          std::map<std::string, std::string> *longTags);

    std::map<std::string, std::string> fetchTags(MP4FileX &file);
}

namespace CAF {
    std::map<std::string, std::string>
        fetchTags(const std::vector<uint8_t> &info);
    std::map<std::string, std::string> fetchTags(int fd);
}

const char * const iTunSMPB_template = " 00000000 %08X %08X %08X%08X "
"00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000";

#endif
