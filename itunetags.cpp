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
	} else if (key == Tag::kTempo) {
	    if (std::sscanf(vp, "%d", &n) == 1)
		m_file.SetMetadataUint16(fourcc(key).svalue, n);
	} else if (key == Tag::kGenreID3) {
	    if (std::sscanf(vp, "%d", &n) == 1)
		m_file.SetMetadataGenre(fourcc(key).svalue, n);
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

namespace id3 {
    const Tag::NameIDMap tagNameMap[] = {
	{ "TIT2", Tag::kTitle },
	{ "TPE1", Tag::kArtist },
	{ "TPE2", Tag::kAlbumArtist },
	{ "TALB", Tag::kAlbum },
	{ "TIT1", Tag::kGrouping },
	{ "TCOM", Tag::kComposer },
	{ "TCON", Tag::kGenre },
	{ "TDRC", Tag::kDate },
	{ "TRCK", Tag::kTrack },
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

uint32_t GetIDFromID3TagName(const char *name)
{
    return id3::GetIDFromName(name, id3::tagNameMap);
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
	boost::shared_ptr<MP4ItmfItemList> __delete_later__(
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

