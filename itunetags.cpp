#include <algorithm>
#include <aifffile.h>
#include <apefile.h>
#include <apetag.h>
#include "id3v1genres.h"
#include "itunetags.h"
#ifdef _WIN32
#include "win32util.h"
#endif
#include "strutil.h"
#include "mp4v2wrapper.h"
#include "wicimage.h"
#include "cuesheet.h"
#include "src/itmf/type.h"
#include "taglibhelper.h"

using mp4v2::impl::MP4File;
using mp4v2::impl::MP4Atom;
using mp4v2::impl::Exception;
using mp4v2::impl::itmf::enumGenreType;
using mp4v2::impl::itmf::enumStikType;
using mp4v2::impl::itmf::enumAccountType;
using mp4v2::impl::itmf::enumCountryCode;
using mp4v2::impl::itmf::enumContentRating;

class ShortTagWriter {
    MP4FileX &m_file;
public:
    ShortTagWriter(MP4FileX &f) : m_file(f) {}
    void operator()(const std::pair<uint32_t, std::wstring> &kv)
    {
	writeTag(kv.first, kv.second);
    }
    void writeTag(uint32_t key, const std::wstring &value)
    {
	std::string sv = strutil::w2us(value);
	util::fourcc fcc(key);

	struct handler_t {
	    uint32_t fcc;
	    void (ShortTagWriter::*mf)(const char *, const char *);
	} handlers[] = {
	    { Tag::kTrack,		&ShortTagWriter::setTrack	},
	    { Tag::kDisk,		&ShortTagWriter::setDisk	},
	    { Tag::kGenre,		&ShortTagWriter::setGenre	},
	    { Tag::kGenreID3,		&ShortTagWriter::setGenre	},
	    { Tag::kCompilation, 	&ShortTagWriter::setInt8	},
	    { Tag::kTempo,		&ShortTagWriter::setInt16	},
	    { Tag::kArtwork,		&ShortTagWriter::ignore		},
	    { Tag::kTvSeason,		&ShortTagWriter::setInt32	},
	    { Tag::kTvEpisode,		&ShortTagWriter::setInt32	},
	    { Tag::kPodcast,		&ShortTagWriter::setInt8	},
	    { Tag::kHDVideo,		&ShortTagWriter::setInt8	},
	    { Tag::kMediaType,		&ShortTagWriter::setMediaType	},
	    { Tag::kContentRating,	&ShortTagWriter::setRating	},
	    { Tag::kGapless,		&ShortTagWriter::setInt8	},
	    { Tag::kiTunesAccountType,	&ShortTagWriter::setAccountType	},
	    { Tag::kiTunesCountry,	&ShortTagWriter::setCountryCode	},
	    { Tag::kcontentID,		&ShortTagWriter::setInt32	},
	    { Tag::kartistID,		&ShortTagWriter::setInt32	},
	    { Tag::kplaylistID,		&ShortTagWriter::setInt64	},
	    { Tag::kgenreID,		&ShortTagWriter::setInt32	},
	    { Tag::kcomposerID,		&ShortTagWriter::setInt32	},
	    { 0,			0				}
	};
	for (handler_t *p = handlers; p->fcc; ++p) {
	    if (fcc == p->fcc) {
		(this->*p->mf)(fcc.svalue, sv.c_str());
		return;
	    }
	}
	setString(fcc.svalue, sv.c_str());
    }
private:
    void setTrack(const char *fcc, const char *value)
    {
	int n, total = 0;
	if (std::sscanf(value, "%d/%d", &n, &total) > 0)
	    m_file.SetMetadataTrack(n, total);
    }
    void setDisk(const char *fcc, const char *value)
    {
	int n, total = 0;
	if (std::sscanf(value, "%d/%d", &n, &total) > 0)
	    m_file.SetMetadataDisk(n, total);
    }
    void setGenre(const char *fcc, const char *value)
    {
	char *endp;
	long n = std::strtol(value, &endp, 10);
	if (endp != value && *endp == 0)
	    m_file.SetMetadataGenre("gnre", n);
	else {
	    n = static_cast<uint16_t>(enumGenreType.toType(value));
	    if (n != mp4v2::impl::itmf::GENRE_UNDEFINED)
		m_file.SetMetadataGenre("gnre", n);
	    else
		m_file.SetMetadataString("\xa9""gen", value);
	}
    }
    void setMediaType(const char *fcc, const char *value)
    {
	unsigned n;
	if (std::scanf("%u", &n) != 1)
	    n = static_cast<uint8_t>(enumStikType.toType(value));
	m_file.SetMetadataUint8(fcc, n);
    }
    void setRating(const char *fcc, const char *value)
    {
	unsigned n;
	if (std::scanf("%u", &n) != 1)
	    n = static_cast<uint8_t>(enumContentRating.toType(value));
	m_file.SetMetadataUint8(fcc, n);
    }
    void setAccountType(const char *fcc, const char *value)
    {
	unsigned n;
	if (std::scanf("%u", &n) != 1)
	    n = static_cast<uint8_t>(enumAccountType.toType(value));
	m_file.SetMetadataUint8(fcc, n);
    }
    void setCountryCode(const char *fcc, const char *value)
    {
	unsigned n;
	if (std::scanf("%u", &n) != 1)
	    n = static_cast<uint8_t>(enumCountryCode.toType(value));
	m_file.SetMetadataUint32(fcc, n);
    }
    void setInt8(const char *fcc, const char *value)
    {
	int n;
	if (std::sscanf(value, "%d", &n) == 1)
	    m_file.SetMetadataUint8(fcc, n);
    }
    void setInt16(const char *fcc, const char *value)
    {
	int n;
	if (std::sscanf(value, "%d", &n) == 1)
	    m_file.SetMetadataUint16(fcc, n);
    }
    void setInt32(const char *fcc, const char *value)
    {
	int n;
	if (std::sscanf(value, "%d", &n) == 1)
	    m_file.SetMetadataUint32(fcc, n);
    }
    void setInt64(const char *fcc, const char *value)
    {
	int64_t n;
	if (std::sscanf(value, "%lld", &n) == 1)
	    m_file.SetMetadataUint64(fcc, n);
    }
    void setString(const char *fcc, const char *value)
    {
	std::string s = strutil::normalize_crlf(value, "\r\n");
	m_file.SetMetadataString(fcc, s.c_str());
    }
    void ignore(const char *fcc, const char *value)
    {
    }
};

class LongTagWriter {
    MP4FileX &m_file;
public:
    LongTagWriter(MP4FileX &f): m_file(f) {}
    void operator()(const std::pair<std::string, std::wstring> &kv)
    {
	writeTag(kv.first, kv.second);
    }
    void writeTag(const std::string &key, const std::wstring &value)
    {
	std::string sv = strutil::w2us(value);
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

void TagEditor::save(MP4FileX &file)
{
    try {
	if (m_nsamples) {
	    std::wstring value = strutil::format(iTunSMPB_template,
				        m_encoder_delay,
				        m_padding,
				        uint32_t(m_nsamples >> 32),
				        uint32_t(m_nsamples & 0xffffffff));
	    m_long_tags["iTunSMPB"] = value;
	}
	if (m_chapters.size()) {
	    uint64_t timeScale = file.GetIntegerProperty("moov.mvhd.timeScale");
	    MP4TrackId track = file.AddChapterTextTrack(1);
	    double off = 0;
	    for (size_t i = 0; i < m_chapters.size(); ++i) {
		std::string name = strutil::w2us(m_chapters[i].first);
		if (name.empty())
		    name = strutil::format("Track %02u",
					   static_cast<uint32_t>(i + 1));
		file.AddChapter(track, m_chapters[i].second * timeScale + 0.5,
				name.c_str());
		int64_t stamp = off * 10000000 + 0.5;
		file.AddNeroChapter(stamp, name.c_str());
		off += m_chapters[i].second;
	    }
	}
	std::for_each(m_tags.begin(), m_tags.end(), ShortTagWriter(file));
	std::for_each(m_long_tags.begin(), m_long_tags.end(),
		      LongTagWriter(file));
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
	    char *data =
		win32::load_with_mmap(m_artworks[i].c_str(), &size);
	    std::shared_ptr<char> dataPtr(data, UnmapViewOfFile);
	    mp4v2::impl::itmf::BasicType tc =
		mp4v2::impl::itmf::computeBasicType(data, size);
	    if (tc == mp4v2::impl::itmf::BT_IMPLICIT)
		throw std::runtime_error("Unknown artwork image type");
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
    void fetchAiffID3Tags(TagLib::RIFF::AIFF::File &file,
			  std::map<uint32_t, std::wstring> *result)
    {
	std::map<uint32_t, std::wstring> tags;
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
			value = strutil::format(L"%d", n + 1);
		    }
		}
		tags[id] = value;
	    }
	}
	result->swap(tags);
    }
    void fetchAiffID3Tags(const wchar_t *filename,
			  std::map<uint32_t, std::wstring> *result)
    {
#ifdef _WIN32
	std::wstring fullname = win32::prefixed_path(filename);
#else
	std::string fullname = strutil::w2m(filename);
#endif
	TagLib::RIFF::AIFF::File file(fullname.c_str(), false);
	if (!file.isOpen())
	    throw std::runtime_error("taglib: can't open file");
	fetchAiffID3Tags(file, result);
    }
    void fetchAiffID3Tags(int fd, std::map<uint32_t, std::wstring> *result)
    {
	util::FilePositionSaver _(fd);
	lseek(fd, 0, SEEK_SET);
	TagLibX::FDIOStreamReader stream(fd);
	TagLib::RIFF::AIFF::File file(&stream, false);
	fetchAiffID3Tags(file, result);
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
	std::string totaltracks, totaldiscs;
	uint32_t id;
	for (it = vc.begin(); it != vc.end(); ++it) {
	    std::string key = strutil::slower(it->first);
	    if (key == "totaltracks" || key == "tracktotal")
		totaltracks = it->second;
	    else if (key == "totaldiscs" || key == "disctotal")
		totaldiscs = it->second;
	    else if ((id = GetIDFromTagName(it->first.c_str())) > 0)
		result[id] = strutil::us2w(it->second);
	}
	if (!totaltracks.empty() && result.find(Tag::kTrack) != result.end()) {
	    result[Tag::kTrack] =
		strutil::format(L"%d/%d", _wtoi(result[Tag::kTrack].c_str()),
		       atoi(totaltracks.c_str()));
	}
	if (!totaldiscs.empty() && result.find(Tag::kDisk) != result.end()) {
	    result[Tag::kDisk] = strutil::format(L"%d/%d",
					_wtoi(result[Tag::kDisk].c_str()),
					atoi(totaldiscs.c_str()));
	}
	itags->swap(result);
    }
    void ConvertFromItunesTags(
	    const std::map<uint32_t, std::wstring> &itags,
	    std::map<std::string, std::string> *vc)
    {
	std::map<uint32_t, std::wstring>::const_iterator ii;
	std::map<std::string, std::string> result;
	for (ii = itags.begin(); ii != itags.end(); ++ii) {
	    const char *name = GetNameFromTagID(ii->first);
	    if (!name) continue;
	    if (ii->first == Tag::kTrack) {
		unsigned n, t = 0;
		if (swscanf(ii->second.c_str(), L"%u/%u", &n, &t) < 1)
		    continue;
		result["tracknumber"] = strutil::format("%u", n);
		if (t > 0) result["tracktotal"] = strutil::format("%u", t);
	    }
	    else if (ii->first == Tag::kDisk) {
		unsigned n, t = 0;
		if (swscanf(ii->second.c_str(), L"%u/%u", &n, &t) < 1)
		    continue;
		result["discnumber"] = strutil::format("%u", n);
		if (t > 0) result["disctotal"] = strutil::format("%u", t);
	    }
	    else if (ii->first == Tag::kGenreID3) {
		unsigned n;
		if (swscanf(ii->second.c_str(), L"%u", &n) == 1) {
		    TagLib::String v = TagLib::ID3v1::genre(n-1);
		    result[name] = v.toCString();
		}
	    }
	    else
		result[name] = strutil::w2us(ii->second);
	}
	vc->swap(result);
    }
}

namespace mp4a
{
    std::wstring parseValue(uint32_t fcc, const MP4ItmfData &data)
    {
	uint8_t *value = data.value;

	if (fcc == Tag::kGenreID3) {
	    unsigned v = (value[0] << 8) | value[1];
	    return strutil::format(L"%u", v);
	} else if (fcc == Tag::kDisk || fcc == Tag::kTrack) {
	    unsigned index = (value[2] << 8) | value[3];
	    unsigned total = (value[4] << 8) | value[5];
	    return strutil::format(L"%u/%u", index, total);
	} else if (data.typeCode == MP4_ITMF_BT_INTEGER) {
	    if (data.valueSize == 8) {
		uint32_t high, low;
		high = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
		low  = (value[4]<<24)|(value[5]<<16)|(value[6]<<8)|value[7];
		uint64_t value = (static_cast<uint64_t>(high) << 32) | low;
		return strutil::format(L"%lld", value);
	    }
	    int v;
	    if (data.valueSize == 1)
		v = value[0];
	    else if (data.valueSize == 2)
		v = (value[0] << 8) | value[1];
	    else if (data.valueSize == 4)
		v = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
	    else
		return L"";
	    return strutil::format(L"%d", v);
	} else if (data.typeCode == MP4_ITMF_BT_UTF8) {
	    char *vp = reinterpret_cast<char*>(value);
	    std::string s(vp, vp + data.valueSize);
	    return strutil::us2w(s);
	}
	return L"";
    }

    void fetchTags(MP4FileX &file, std::map<uint32_t, std::wstring> *shortTags,
		   std::map<std::string, std::wstring> *longTags)
    {
	std::map<uint32_t, std::wstring> stags;
	std::map<std::string, std::wstring> ltags;
	try {
	    MP4ItmfItemList *itemlist =
		mp4v2::impl::itmf::genericGetItems(file);
	    if (!itemlist)
		return;
	    std::shared_ptr<MP4ItmfItemList> __delete_later__(
		    itemlist, mp4v2::impl::itmf::genericItemListFree);
	    for (size_t i = 0; i < itemlist->size; ++i) {
		MP4ItmfItem &item = itemlist->elements[i];
		uint32_t fcc = util::fourcc(item.code);
		MP4ItmfData &data = item.dataList.elements[0];
		if (!data.value)
		    continue;
		std::wstring v = parseValue(fcc, data);
		if (!v.empty()) {
		    if (fcc == '----')
			ltags[item.name] = v;
		    else
			stags[fcc] = v;
		}
	    }
	    if (shortTags) shortTags->swap(stags);
	    if (longTags) longTags->swap(ltags);
	} catch (mp4v2::impl::Exception *e) {
	    handle_mp4error(e);
	}
    }
}
