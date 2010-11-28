#ifndef ITUNES_TAGS_H
#define ITUNES_TAGS_H

#include <iterator>
#include <id3tag.h>
#include "utf8_codecvt_facet.hpp"
#include "iointer.h"
#include "iff.h"

namespace Tag {
    const uint32_t kTitle = fourcc("\xa9""nam");
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

uint32_t GetIDFromTagName(const char *name);
const char *GetNameFromTagID(uint32_t fcc);

const char * const iTunSMPB_template = " 00000000 %08X %08X %08X%08X "
"00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000";

class TagEditor {
    std::wstring m_filename;
    std::map<uint32_t, std::wstring> m_tags;
    std::map<std::string, std::wstring> m_long_tags;
    std::vector<std::pair<std::wstring, int64_t> > m_chapters;
    int m_encoder_delay;
    int m_padding;
    uint64_t m_nsamples;
public:
    TagEditor(const std::wstring &path)
	: m_filename(path),
	  m_encoder_delay(0),
	  m_padding(0),
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
	std::copy(tags.begin(), tags.end(),
		std::insert_iterator<std::map<uint32_t, std::wstring> >(
		    m_tags, m_tags.begin()));
    }
    void setChapters(const std::vector<std::pair<std::wstring, int64_t> >
	    &chapters)
    {
	m_chapters = chapters;
    }
    void save();
};

struct HINSTANCE__;

class LibID3TagModule {
    typedef std::tr1::shared_ptr<HINSTANCE__> module_t;
    module_t m_module;
    bool m_loaded;
public:
    LibID3TagModule(): m_loaded(false) {}
    LibID3TagModule(const std::wstring &path);
    bool loaded() const { return m_loaded; }

    struct id3_tag *(*tag_parse)(id3_byte_t const *, id3_length_t);
    void (*tag_delete)(struct id3_tag *);
    id3_length_t (*ucs4_utf16size)(id3_ucs4_t const *);
    void (*utf16_encode)(id3_utf16_t *, id3_ucs4_t const *);
    id3_ucs4_t const *(*field_getfullstring)(union id3_field const *);
    id3_ucs4_t const *(*field_getstrings)(union id3_field const *, unsigned);
};

class AIFFTagParser: public ITagParser, private IFFParser {
    enum { MAX_ID3_LEN = 0x100000 }; /* 1MB limit is enough? */
    LibID3TagModule m_module;
    std::map<uint32_t, std::wstring> m_tags;
public:
    AIFFTagParser(const LibID3TagModule &module, InputStream &stream);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const { return 0; }
private:
    void parseTags();
    void fetch(id3_tag *parser);
    std::wstring ucs4_to_wstring(const id3_ucs4_t *us);
};

class M4ATagParser : public ITagParser {
    std::map<uint32_t, std::wstring> m_tags;
    std::map<std::string, std::wstring> m_long_tags;
    bool m_is_alac;
public:
    M4ATagParser(const std::wstring &filename);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::map<std::string, std::wstring> &getLongTags() const
    {
	return m_long_tags;
    }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const { return 0; }
    bool isALAC() const { return m_is_alac; }
};

bool
TransVorbisComment(const char *kv,
	std::pair<uint32_t, std::wstring> *result, std::string *key=0);

#endif
