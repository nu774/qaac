#include <algorithm>
#include "itunetags.h"
#include "win32util.h"
#include "strcnv.h"
#include "mp4v2wrapper.h"

using mp4v2::impl::MP4File;
using mp4v2::impl::MP4Atom;
using mp4v2::impl::MP4Error;

class ShortTagWriter {
    typedef std::codecvt<wchar_t, char, std::mbstate_t> codec_t;
    MP4FileX &m_file;
    codec_t &m_codec;
public:
    ShortTagWriter(MP4FileX &f, codec_t &codec)
	: m_file(f), m_codec(codec) {}
    void operator()(const std::pair<uint32_t, std::wstring> &kv)
    {
	writeTag(kv.first, kv.second);
    }
    void writeTag(uint32_t key, const std::wstring &value)
    {
	std::string sv = w2m(value, m_codec);
	const char *vp = sv.c_str();
	int n, total = 0;

	if (key == Tag::kTrack) {
	    if (std::sscanf(vp, "%d/%d", &n, &total) > 0)
		m_file.SetMetadataTrack(n, total);
	} else if (key == Tag::kDisk) {
	    if (std::sscanf(vp, "%d/%d", &n, &total) > 0)
		m_file.SetMetadataDisk(n, total);
	} else if (key == Tag::kCompilation) {
	    if (std::sscanf(vp, "%d", &n) == 1)
		m_file.SetMetadataUint8(fourcc(key).svalue, n);
	} else if (key == Tag::kGenreID3 || key == Tag::kTempo) {
	    if (std::sscanf(vp, "%d", &n) == 1)
		m_file.SetMetadataUint16(fourcc(key).svalue, n);
	} else
	    m_file.SetMetadataString(fourcc(key).svalue, vp);
    }
};

class LongTagWriter {
    typedef std::codecvt<wchar_t, char, std::mbstate_t> codec_t;
    MP4FileX &m_file;
    codec_t &m_codec;
public:
    LongTagWriter(MP4FileX &f, codec_t &codec)
	: m_file(f), m_codec(codec) {}
    void operator()(const std::pair<std::string, std::wstring> &kv)
    {
	writeTag(kv.first, kv.second);
    }
    void writeTag(const std::string &key, const std::wstring &value)
    {
	std::string sv = w2m(value, m_codec);
	try {
	    m_file.SetMetadataFreeForm(key.c_str(),
		"com.apple.iTunes",
		reinterpret_cast<const uint8_t*>(sv.c_str()),
		sv.size());
	} catch (MP4Error *e) {
	    handle_mp4error(e);
	}
    }
};

void TagEditor::save()
{
    try {
	if (m_nsamples) {
	    std::string value = format(iTunSMPB_template,
				    m_encoder_delay,
				    m_padding,
				    int32_t(m_nsamples >> 32),
				    int32_t(m_nsamples & 0xffffffff));
	    m_long_tags["iTunSMPB"] = widen(value);
	}
	utf8_codecvt_facet u8codec;
	std::string utf8_name = w2m(m_filename, u8codec);

	MP4FileX file(0);
	file.Modify(utf8_name.c_str());

	if (m_chapters.size()) {
	    uint64_t timeScale = file.GetIntegerProperty("moov.mvhd.timeScale");
	    MP4TrackId track = file.AddChapterTextTrack(1);
	    int64_t samples = 0;
	    for (size_t i = 0; i < m_chapters.size(); ++i) {
		std::string name = w2m(m_chapters[i].first, u8codec);
		file.AddChapter(track, m_chapters[i].second, name.c_str());
		int64_t stamp = static_cast<double>(samples)
			* 10000000 / timeScale;
		file.AddNeroChapter(stamp, name.c_str());
		samples += m_chapters[i].second;
	    }
	}
	std::for_each(m_tags.begin(), m_tags.end(),
		ShortTagWriter(file, u8codec));
	std::for_each(m_long_tags.begin(), m_long_tags.end(),
		LongTagWriter(file, u8codec));

	file.Close();
    } catch (MP4Error *e) {
	handle_mp4error(e);
    }
}

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("ERROR"); } \
    while (0)

LibID3TagModule::LibID3TagModule(const std::wstring &path)
{
    HMODULE hDll;
    hDll = LoadLibraryW(path.c_str());
    m_loaded = (hDll != NULL);
    if (!m_loaded)
	return;
    try {
	CHECK(tag_parse = ProcAddress(hDll, "id3_tag_parse"));
	CHECK(tag_delete = ProcAddress(hDll, "id3_tag_delete"));
	CHECK(ucs4_utf16size = ProcAddress(hDll, "id3_ucs4_utf16size"));
	CHECK(utf16_encode = ProcAddress(hDll, "id3_utf16_encode"));
	CHECK(field_getfullstring =
		ProcAddress(hDll, "id3_field_getfullstring"));
	CHECK(field_getstrings = ProcAddress(hDll, "id3_field_getstrings"));
    } catch (...) {
	FreeLibrary(hDll);
	m_loaded = false;
	return;
    }
    m_module.swap(module_t(hDll, FreeLibrary));
}

namespace id3 {
    const Tag::NameIDMap tagNameMap[] = {
	{ ID3_FRAME_TITLE, Tag::kTitle },
	{ ID3_FRAME_ARTIST, Tag::kArtist },
	{ "TPE2", Tag::kAlbumArtist },
	{ ID3_FRAME_ALBUM, Tag::kAlbum },
	{ "TIT1", Tag::kGrouping },
	{ "TCOM", Tag::kComposer },
	{ ID3_FRAME_GENRE, Tag::kGenre },
	{ ID3_FRAME_YEAR, Tag::kDate },
	{ ID3_FRAME_TRACK, Tag::kTrack },
	{ "TPOS", Tag::kDisk },
	{ "TBPM", Tag::kTempo },
	{ "TCOP", Tag::kCopyright },
	{ "TCMP", Tag::kCompilation },
	{ 0, 0 }
    };
    uint32_t GetIDFromName(const char *name, const Tag::NameIDMap *map) {
	for (; map->name; ++map)
	    if (!std::strcmp(map->name, name))
		return map->id;
	return 0;
    }
}

AIFFTagParser::AIFFTagParser(
	const LibID3TagModule &module, InputStream &stream)
    : m_module(module), IFFParser(stream)
{
    parse();
    if (format_id() != 'AIFF')
	throw std::runtime_error("AIFFTagParser: Not an AIFF file");
    while (next() && chunk_id() != 'ID3 ')
	;
    if (chunk_id() == 'ID3 ' && chunk_size() < MAX_ID3_LEN)
	parseTags();
}

void AIFFTagParser::parseTags()
{
    size_t size = static_cast<size_t>(chunk_size());
    std::vector<id3_byte_t> buffer(size);
    read(&buffer[0], size);
    id3_tag *tag = m_module.tag_parse(&buffer[0], size);
    if (!tag)
	throw std::runtime_error("id3_tag_parse: can't parse as ID3 tag");
    std::tr1::shared_ptr<id3_tag> __delete_later__(tag, m_module.tag_delete);
    fetch(tag);
}

void AIFFTagParser::fetch(id3_tag *tag)
{
    for (size_t i = 0; i < tag->nframes; ++i) {
	id3_frame *frame = tag->frames[i];
	uint32_t id = id3::GetIDFromName(frame->id, id3::tagNameMap);
	if (!id || frame->nfields != 2)
	    continue;
	id3_field *field = &frame->fields[1];
	const id3_ucs4_t *us = m_module.field_getfullstring(field);
	if (!us)
	    us = m_module.field_getstrings(field, 0);
	if (!us) continue;
	std::wstring ws = ucs4_to_wstring(us);
	if (id == Tag::kGenre) {
	    /* check if it is a genre number */
	    wchar_t *endp;
	    long n = std::wcstol(ws.c_str(), &endp, 10);
	    if (*endp == 0 && n >= 0 && n < 126) {
		/* iTune M4A's genre number begins from 1 */
		id = Tag::kGenreID3;
		ws = widen(format("%d", n + 1));
	    }
	}
	m_tags.insert(std::make_pair(id, ws));
    }
}

std::wstring AIFFTagParser::ucs4_to_wstring(const id3_ucs4_t *ustr)
{
    std::vector<id3_utf16_t> u16(m_module.ucs4_utf16size(ustr));
    m_module.utf16_encode(&u16[0], ustr);
    return std::wstring(u16.begin(), u16.end());
}

namespace __m4a {
    std::wstring parseValue(uint32_t fcc, const MP4ItmfData &data,
	    std::codecvt<wchar_t, char, std::mbstate_t> &codec)
    {
	if (!data.value)
	    return L"";
	uint8_t *value = data.value;

	if (fcc == Tag::kGenreID3) {
	    int v = (value[0] << 8) | value[1];
	    return widen(format("%d", v));
	} else if (fcc == Tag::kDisk || fcc == Tag::kTrack) {
	    int index = (value[2] << 8) | value[3];
	    int total = (value[4] << 8) | value[5];
	    return widen(format("%d/%d", index, total));
	} else if (data.typeCode == MP4_ITMF_BT_INTEGER) {
	    int v;
	    if (data.valueSize == 1)
		v = value[0];
	    else if (data.valueSize == 2)
		v = (value[0] << 8) | value[1];
	    else if (data.valueSize == 4)
		v = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
	    else
		return L"";
	    return widen(format("%d", v));
	} else if (data.typeCode == MP4_ITMF_BT_UTF8) {
	    char *vp = reinterpret_cast<char*>(value);
	    std::string s(vp, vp + data.valueSize);
	    return m2w(s, codec);
	}
	return L"";
    }
}

M4ATagParser::M4ATagParser(const std::wstring &filename)
{
    try {
	utf8_codecvt_facet u8codec;
	std::string utf8_name = w2m(filename, u8codec);

	MP4File file(0);
	file.Read(utf8_name.c_str(), 0);
	MP4TrackId track_id = file.FindTrackId(0, MP4_AUDIO_TRACK_TYPE);
	MP4Atom *atom
	    = file.FindTrackAtom(track_id, "mdia.minf.stbl.stsd.alac");
	m_is_alac = !!atom;

	MP4ItmfItemList *itemlist = mp4v2::impl::itmf::genericGetItems(file);
	if (!itemlist)
	    return;
	std::tr1::shared_ptr<MP4ItmfItemList> __delete_later__(
		itemlist, mp4v2::impl::itmf::genericItemListFree);
	for (size_t i = 0; i < itemlist->size; ++i) {
	    MP4ItmfItem &item = itemlist->elements[i];
	    uint32_t fcc = fourcc(item.code);
	    MP4ItmfData &data = item.dataList.elements[0];
	    if (!data.value)
		continue;
	    if (fcc == '----') {
		const char *mp = reinterpret_cast<const char *>(item.mean);
		const char *np = reinterpret_cast<const char *>(item.name);
		if (!mp|| !np || std::strcmp(mp, "com.apple.iTunes"))
		    continue;
		std::wstring v = __m4a::parseValue(fcc, data, u8codec);
		if (!v.empty()) m_long_tags[np] = v;
	    } else {
		std::wstring v = __m4a::parseValue(fcc, data, u8codec);
		if (!v.empty()) m_tags[fcc] = v;
	    }
	}
    } catch (MP4Error *e) {
	handle_mp4error(e);
    }
}

namespace __freeform {
    const Tag::NameIDMap tagNameMap[] = {
	{ "title", Tag::kTitle },
	{ "artist", Tag::kArtist },
	{ "albumartist", Tag::kAlbumArtist },
	{ "album", Tag::kAlbum },
	{ "grouping", Tag::kGrouping },
	{ "composer", Tag::kComposer },
	{ "genre", Tag::kGenre },
	{ "date", Tag::kDate },
	{ "year", Tag::kDate },
	{ "tracknumber", Tag::kTrack },
	{ "track", Tag::kTrack },
	{ "discnumber", Tag::kDisk },
	{ "disc", Tag::kDisk },
	{ "comment", Tag::kComment },
	{ 0, 0 }
    };
    uint32_t GetIDFromName(const char *name, const Tag::NameIDMap *map) {
	for (; map->name; ++map)
	    if (!strcasecmp(map->name, name))
		return map->id;
	return 0;
    }
    const char *GetNameFromID(uint32_t fcc, const Tag::NameIDMap *map) {
	for (; map->name; ++map)
	    if (map->id == fcc)
		return map->name;
	return 0;
    }
}

uint32_t GetIDFromTagName(const char *name)
{
    return __freeform::GetIDFromName(name, __freeform::tagNameMap);
}

const char *GetNameFromTagID(uint32_t fcc)
{
    return __freeform::GetNameFromID(fcc, __freeform::tagNameMap);
}

bool
TransVorbisComment(const char *kv, std::pair<uint32_t, std::wstring> *result,
	std::string *key)
{
    std::vector<char> buff(kv, kv + std::strlen(kv) + 1);
    char *value = &buff[0];
    char *k = strsep(&value, "=");
    uint32_t id = GetIDFromTagName(k);
    if (key) *key = k;
    *result = std::make_pair(id, m2w(value, utf8_codecvt_facet()));
    return id != 0;
}

