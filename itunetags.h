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
    const uint32_t kArtwork = fourcc("covr");

    struct NameIDMap {
	const char *name;
	uint32_t id;
    };
}

namespace ID3 {
    uint32_t GetIDFromTagName(const char *name);
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
    void fetchAiffID3Tags(const wchar_t *filename);
    void fetchAPETags(const wchar_t *filename, uint32_t sampleRate,
	    uint64_t duration);
    void save(MP4FileX &mp4file);
    void saveArtworks(MP4FileX &mp4file);
};

#endif
