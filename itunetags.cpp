#include <algorithm>
#include <aifffile.h>
#include <apefile.h>
#include <apetag.h>
#include "id3v1genres.h"
#include "itunetags.h"
#ifdef _WIN32
#include "win32util.h"
#endif
#include "strcnv.h"
#include "mp4v2wrapper.h"
#include "wicimage.h"
#include "cuesheet.h"

using mp4v2::impl::MP4File;
using mp4v2::impl::MP4Atom;
using mp4v2::impl::Exception;

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
	} else if (key == Tag::kGenre) {
	    char *endp;
	    long v = std::strtol(vp, &endp, 10);
	    if (endp != vp && *endp == 0)
		m_file.SetMetadataGenre("gnre", v + 1);
	    else
		m_file.SetMetadataString(fourcc(key).svalue, vp);
	} else if (key == Tag::kLyrics) {
	    std::string s = normalize_crlf(vp, "\r\n");
	    m_file.SetMetadataString(fourcc(key).svalue, s.c_str());
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
	} catch (Exception *e) {
	    handle_mp4error(e);
	}
    }
};

void TagEditor::fetchAiffID3Tags(const wchar_t *filename)
{
#ifdef _WIN32
    std::wstring fullname = get_prefixed_fullpath(filename);
#else
    std::string fullname = w2m(filename);
#endif
    TagLib::RIFF::AIFF::File file(fullname.c_str());
    if (!file.isOpen())
	throw std::runtime_error("taglib: can't open file");
    TagLib::ID3v2::Tag *tag = file.tag();
    const TagLib::ID3v2::FrameList &frameList = tag->frameList();
    TagLib::ID3v2::FrameList::ConstIterator it;
    for (it = frameList.begin(); it != frameList.end(); ++it) {
	TagLib::ByteVector vID = (*it)->frameID();
	std::string sID(vID.data(), vID.data() + vID.size());
	std::wstring value = (*it)->toString().toWString();
	uint32_t id = ID3::GetIDFromTagName(sID.c_str());
	if (id) {
	    if (id == Tag::kGenre) {
		wchar_t *endp;
		long n = std::wcstol(value.c_str(), &endp, 10);
		if (!*endp) {
		    id = Tag::kGenreID3;
		    value = widen(format("%d", n + 1));
		}
	    }
	    setTag(id, value);
	}
    }
}

void TagEditor::fetchAPETags(const wchar_t *filename, uint32_t sampleRate,
	uint64_t duration)
{
#ifdef _WIN32
    std::wstring fullname = get_prefixed_fullpath(filename);
#else
    std::string fullname = w2m(filename);
#endif
    TagLib::APE::File file(fullname.c_str(), false);
    if (!file.isOpen())
	throw std::runtime_error("taglib: can't open file");
    TagLib::APE::Tag *tag = file.APETag(false);
    const TagLib::APE::ItemListMap &itemListMap = tag->itemListMap();
    TagLib::APE::ItemListMap::ConstIterator it;
    for (it = itemListMap.begin(); it != itemListMap.end(); ++it) {
	std::wstring key = it->first.toWString();
	std::string skey = nallow(key);
	std::wstring value = it->second.toString().toWString();
	uint32_t id = Vorbis::GetIDFromTagName(skey.c_str());
	if (id) setTag(id, value);
	if (!strcasecmp(skey.c_str(), "cuesheet")) {
	    std::map<uint32_t, std::wstring> meta;
	    Cue::CueSheetToChapters(value, sampleRate, duration,
		    &m_chapters, &meta);
	    setTag(meta);
	}
    }
}

void TagEditor::save(MP4FileX &file)
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

	if (m_chapters.size()) {
	    uint64_t timeScale = file.GetIntegerProperty("moov.mvhd.timeScale");
	    MP4TrackId track = file.AddChapterTextTrack(1);
	    int64_t samples = 0;
	    for (size_t i = 0; i < m_chapters.size(); ++i) {
		std::string name = w2m(m_chapters[i].first, u8codec);
		file.AddChapter(track, m_chapters[i].second, name.c_str());
		int64_t stamp = static_cast<double>(samples)
			* 10000000 / timeScale + 0.5;
		file.AddNeroChapter(stamp, name.c_str());
		samples += m_chapters[i].second;
	    }
	}
	std::for_each(m_tags.begin(), m_tags.end(),
		ShortTagWriter(file, u8codec));
	std::for_each(m_long_tags.begin(), m_long_tags.end(),
		LongTagWriter(file, u8codec));
    } catch (Exception *e) {
	handle_mp4error(e);
    }
}

void TagEditor::saveArtworks(MP4FileX &file)
{
#ifdef _WIN32
    try {
	for (size_t i = 0; i < m_artworks.size(); ++i) {
	    uint64_t size;
	    char *data = load_with_mmap(m_artworks[i].c_str(), &size);
	    x::shared_ptr<char> dataPtr(data, UnmapViewOfFile);
	    std::vector<char> vec;
	    if (m_artwork_size) {
		bool res = WICConvertArtwork(data, size, m_artwork_size, &vec);
		if (res) {
		    data = &vec[0];
		    size = vec.size();
		}
	    }
	    file.SetMetadataArtwork("covr", data, size);
	}
    } catch (Exception *e) {
	handle_mp4error(e);
    }
#endif
}

namespace ID3 {
    const Tag::NameIDMap tagNameMap[] = {
	{ "TIT1", Tag::kGrouping },
	{ "TIT2", Tag::kTitle },
	{ "TIT3", Tag::kSubTitle },
	{ "TPE1", Tag::kArtist },
	{ "TPE2", Tag::kAlbumArtist },
	{ "TALB", Tag::kAlbum },
	{ "TCOM", Tag::kComposer },
	{ "TCON", Tag::kGenre },
	{ "TDRC", Tag::kDate },
	{ "TRCK", Tag::kTrack },
	{ "TPOS", Tag::kDisk },
	{ "TBPM", Tag::kTempo },
	{ "TCOP", Tag::kCopyright },
	{ "TCMP", Tag::kCompilation },
	{ "USLT", Tag::kLyrics },
	{ 0, 0 }
    };
    uint32_t GetIDFromTagName(const char *name) {
	const Tag::NameIDMap *map = tagNameMap;
	for (; map->name; ++map)
	    if (!std::strcmp(map->name, name))
		return map->id;
	return 0;
    }
}

namespace Vorbis {
    const Tag::NameIDMap tagNameMap[] = {
	{ "title", Tag::kTitle },
	{ "artist", Tag::kArtist },
	{ "albumartist", Tag::kAlbumArtist },
	{ "album artist", Tag::kAlbumArtist },
	{ "album", Tag::kAlbum },
	{ "grouping", Tag::kGrouping },
	{ "composer", Tag::kComposer },
	{ "genre", Tag::kGenre },
	{ "genre", Tag::kGenreID3 },
	{ "date", Tag::kDate },
	{ "year", Tag::kDate },
	{ "tracknumber", Tag::kTrack },
	{ "track", Tag::kTrack },
	{ "discnumber", Tag::kDisk },
	{ "disc", Tag::kDisk },
	{ "comment", Tag::kComment },
	{ "subtitle", Tag::kSubTitle },
	{ "lyrics", Tag::kLyrics },
	{ 0, 0 }
    };
    uint32_t GetIDFromTagName(const char *name)
    {
	const Tag::NameIDMap *map = tagNameMap;
	for (; map->name; ++map)
	    if (!strcasecmp(map->name, name))
		return map->id;
	return 0;
    }

    const char *GetNameFromTagID(uint32_t fcc)
    {
	const Tag::NameIDMap *map = tagNameMap;
	for (; map->name; ++map)
	    if (map->id == fcc)
		return map->name;
	return 0;
    }
    void ConvertToItunesTags(
	    const std::map<std::string, std::string> &vc,
	    std::map<uint32_t, std::wstring> *itags)
    {
	std::map<std::string, std::string>::const_iterator it;
	std::map<uint32_t, std::wstring> result;
	utf8_codecvt_facet u8codec;
	std::string totaltracks, totaldiscs;
	uint32_t id;
	for (it = vc.begin(); it != vc.end(); ++it) {
	    std::string key = slower(it->first);
	    if (key == "totaltracks" || key == "tracktotal")
		totaltracks = it->second;
	    else if (key == "totaldiscs" || key == "disctotal")
		totaldiscs = it->second;
	    else if ((id = GetIDFromTagName(it->first.c_str())) > 0)
		result[id] = m2w(it->second, u8codec);
	}
	if (!totaltracks.empty() && result.find(Tag::kTrack) != result.end()) {
	    result[Tag::kTrack] = widen(format("%d/%d",
		_wtoi(result[Tag::kTrack].c_str()),
		atoi(totaltracks.c_str())));
	}
	if (!totaldiscs.empty() && result.find(Tag::kDisk) != result.end()) {
	    result[Tag::kDisk] = widen(format("%d/%d",
		_wtoi(result[Tag::kDisk].c_str()),
		atoi(totaldiscs.c_str())));
	}
	itags->swap(result);
    }
    void ConvertFromItunesTags(
	    const std::map<uint32_t, std::wstring> &itags,
	    std::map<std::string, std::string> *vc)
    {
	std::map<uint32_t, std::wstring>::const_iterator ii;
	std::map<std::string, std::string> result;
	utf8_codecvt_facet u8codec;
	for (ii = itags.begin(); ii != itags.end(); ++ii) {
	    const char *name = GetNameFromTagID(ii->first);
	    if (!name) continue;
	    if (ii->first == Tag::kTrack) {
		int n, t = 0;
		if (swscanf(ii->second.c_str(), L"%d/%d", &n, &t) < 1)
		    continue;
		result["tracknumber"] = format("%d", n);
		if (t > 0) result["tracktotal"] = format("%d", t);
	    }
	    else if (ii->first == Tag::kDisk) {
		int n, t = 0;
		if (swscanf(ii->second.c_str(), L"%d/%d", &n, &t) < 1)
		    continue;
		result["discnumber"] = format("%d", n);
		if (t > 0) result["disctotal"] = format("%d", t);
	    }
	    else if (ii->first == Tag::kGenreID3) {
		int n;
		if (swscanf(ii->second.c_str(), L"%d", &n) == 1) {
		    TagLib::String v = TagLib::ID3v1::genre(n-1);
		    result[name] = v.toCString();
		}
	    }
	    else
		result[name] = w2m(ii->second, u8codec);
	}
	vc->swap(result);
    }
}

namespace mp4a
{
    std::wstring parseValue(uint32_t fcc, const MP4ItmfData &data,
	    std::codecvt<wchar_t, char, std::mbstate_t> &codec)
    {
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

    void fetchTags(MP4FileX &file, std::map<uint32_t, std::wstring> *result)
    {
	utf8_codecvt_facet u8codec;
	std::map<uint32_t, std::wstring> tags;
	try {
	    MP4ItmfItemList *itemlist =
		mp4v2::impl::itmf::genericGetItems(file);
	    if (!itemlist)
		return;
	    x::shared_ptr<MP4ItmfItemList> __delete_later__(
		    itemlist, mp4v2::impl::itmf::genericItemListFree);
	    for (size_t i = 0; i < itemlist->size; ++i) {
		MP4ItmfItem &item = itemlist->elements[i];
		uint32_t fcc = fourcc(item.code);
		MP4ItmfData &data = item.dataList.elements[0];
		if (!data.value)
		    continue;
		if (fcc != '----') {
		    std::wstring v = parseValue(fcc, data, u8codec);
		    if (!v.empty()) tags[fcc] = v;
		}
	    }
	    result->swap(tags);
	} catch (mp4v2::impl::Exception *e) {
	    handle_mp4error(e);
	}
    }
}
