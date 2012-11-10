#include <sstream>
#include <clocale>
#include <locale>
#include "cuesheet.h"
#include "itunetags.h"
#include "composite.h"
#include "nullsource.h"
#include "TrimmedSource.h"
#include "expand.h"
#include "inputfactory.h"

static inline
unsigned msf2frames(unsigned mm, unsigned ss, unsigned ff)
{
    return (mm * 60 + ss) * 75 + ff;
}

static inline
uint64_t frame2sample(uint32_t sampling_rate, uint32_t nframe)
{
    return static_cast<uint64_t>(nframe / 75.0 * sampling_rate + 0.5);
}

template <typename CharT>
bool CueTokenizer<CharT>::nextline()
{
    m_fields.clear();
    int_type c;
    std::basic_string<CharT> field;
    while (traits_type::not_eof(c = m_sb->sbumpc())) {
	if (c == '"') {
	    // eat until closing quote
	    while (traits_type::not_eof(c = m_sb->sbumpc())) {
		if (c == '\n')
		    throw std::runtime_error(strutil::format(
			"cuesheet: runaway string at line %u",
			static_cast<uint32_t>(m_lineno) + 1));
		else if (c != '"')
		    field.push_back(c);
		else if (m_sb->sgetc() != '"') // closing quote
		    break;
		else { // escaped quote
		    m_sb->snextc();
		    field.push_back(c);
		}
	    }
	}
	else if (c == '\n') {
	    ++m_lineno;
	    break;
	}
	else if (isWS(c)) {
	    if (field.size()) {
		m_fields.push_back(field);
		field.clear();
	    }
	    while (isWS(m_sb->sgetc()))
		m_sb->snextc();
	}
	else
	    field.push_back(c);
    }
    if (field.size()) m_fields.push_back(field);
    return field.size() > 0 || c == '\n';
}

template struct CueTokenizer<char>;
template struct CueTokenizer<wchar_t>;

void CueSheet::parse(std::wstreambuf *src)
{
    static struct handler_t {
	const wchar_t *cmd;
	void (CueSheet::*mf)(const std::wstring *args);
	size_t nargs;
    } handlers[] = {
	{ L"FILE", &CueSheet::parseFile, 3 },
	{ L"TRACK", &CueSheet::parseTrack, 3 },
	{ L"INDEX", &CueSheet::parseIndex, 3 },
	{ L"POSTGAP", &CueSheet::parsePostgap, 2 },
	{ L"PREGAP", &CueSheet::parsePregap, 2 },
	{ L"REM", &CueSheet::parseRem, 3 },
	{ L"CATALOG", &CueSheet::parseMeta, 2 },
	{ L"ISRC", &CueSheet::parseMeta, 2 },
	{ L"PERFORMER", &CueSheet::parseMeta, 2 },
	{ L"SONGWRITER", &CueSheet::parseMeta, 2 },
	{ L"TITLE", &CueSheet::parseMeta, 2 },
	{ 0, 0, 0 }
    };

    CueTokenizer<wchar_t> tokenizer(src);
    while (tokenizer.nextline()) {
	if (!tokenizer.m_fields.size())
	    continue;
	m_lineno = tokenizer.m_lineno;
	std::wstring cmd = tokenizer.m_fields[0];
	for (handler_t *p = handlers; p->cmd; ++p) {
	    if (cmd != p->cmd)
		continue;
	    if (tokenizer.m_fields.size() == p->nargs)
		(this->*p->mf)(&tokenizer.m_fields[0]);
	    else if (cmd != L"REM")
		die(strutil::format("wrong num args for %ls command", p->cmd));
	    break;
	}
	// if (!p->cmd) die("Unknown command");
    }
    arrange();
}

struct TagLookup {
    typedef std::map<uint32_t, std::wstring> meta_t;
    const CueTrack &track;
    const meta_t &tracktags;

    TagLookup(const CueTrack &track_, const meta_t &tags)
	: track(track_), tracktags(tags) {}

    std::wstring operator()(const std::wstring &name) {
	std::wstring namex = strutil::wslower(name);
	if (namex == L"tracknumber")
	    return strutil::format(L"%02d", track.number());
	std::string skey = strutil::w2us(namex);
	uint32_t id = Vorbis::GetIDFromTagName(skey.c_str());
	if (id == 0) return L"";
	meta_t::const_iterator iter = tracktags.find(id);
	return iter == tracktags.end() ? L"" : iter->second;
    }
};

void CueSheet::loadTracks(std::vector<chapters::Track> &tracks,
			  const std::wstring &cuedir,
			  const std::wstring &fname_format)
{
    typedef std::map<uint32_t, std::wstring> meta_t;
    meta_t album_tags;
    Cue::ConvertToItunesTags(m_meta, &album_tags, true);

    for (iterator track = begin(); track != end(); ++track) {
	meta_t track_tags;
	{
	    meta_t tmp;
	    Cue::ConvertToItunesTags(track->meta(), &track_tags);
	    track_tags.insert(album_tags.begin(), album_tags.end());
	    track_tags[Tag::kTrack] =
		strutil::format(L"%d/%d", track->number(), count());
	}
	std::shared_ptr<CompositeSource> track_source(new CompositeSource());
	track_source->setTags(track_tags);
	std::shared_ptr<ISeekableSource> src;
	for (CueTrack::iterator
	     segment = track->begin(); segment != track->end(); ++segment)
	{
	    if (segment->m_filename == L"__GAP__") {
		if (!src.get()) continue;
		src.reset(new NullSource(src->getSampleFormat()));
	    } else {
		std::wstring ifilename =
		    win32::PathCombineX(cuedir, segment->m_filename);
		src = input::factory()->open(ifilename.c_str());
	    }
	    unsigned rate = src->getSampleFormat().mSampleRate;
	    int64_t begin = frame2sample(rate, segment->m_begin);
	    int64_t duration = -1;
	    if (segment->m_end != -1)
		duration = frame2sample(rate, segment->m_end) - begin;
	    src.reset(new TrimmedSource(src, begin, duration));
	    track_source->addSource(src);
	}

	std::wstring ofilename =
	    process_template(fname_format, TagLookup(*track, track_tags));
	struct F {
	    static wchar_t trans(wchar_t ch) {
		return std::wcschr(L":/\\?|<>*\"", ch) ? L'_' : ch;
	    }
	};
	ofilename = strutil::strtransform(ofilename, F::trans) + L".stub";

	chapters::Track new_track;
	new_track.name = track->name();
	new_track.source = track_source;
	new_track.ofilename = ofilename;
	tracks.push_back(new_track);
    }
}

void CueSheet::arrange()
{
    for (size_t i = 0; i < m_tracks.size(); ++i) {
	bool index1_found = false;
	int64_t last_index = -1;
	CueTrack &track = m_tracks[i];
	for (size_t j = 0; j < track.m_segments.size(); ++j) {
	    if (last_index >= track.m_segments[j].m_index)
		throw std::runtime_error(strutil::format("cuesheet: "
						"INDEX must be in "
						"strictly ascending order: "
						"track %u", track.m_number));
	    last_index = track.m_segments[j].m_index;
	    if (last_index == 1) index1_found = true;
	}
	if (!index1_found)
	    throw std::runtime_error(strutil::format("cuesheet: "
						     "INDEX01 not found on "
						     "track %u",
						     track.m_number));
    }
    /* move INDEX00 segment to previous track's end */
    for (size_t i = 0; i < m_tracks.size(); ++i) {
	if (m_tracks[i].m_segments[0].m_index == 0) {
	    if (i > 0)
		m_tracks[i-1].m_segments.push_back(m_tracks[i].m_segments[0]);
	    m_tracks[i].m_segments.erase(m_tracks[i].m_segments.begin());
	}
    }
    /* join continuous segments */
    for (size_t i = 0; i < m_tracks.size(); ++i) {
	std::vector<CueSegment> segments;
	for (size_t j = 0; j < m_tracks[i].m_segments.size(); ++j) {
	    CueSegment &seg = m_tracks[i].m_segments[j];
	    if (!segments.size()) {
		segments.push_back(seg);
		continue;
	    }
	    CueSegment &last = segments.back();
	    if (last.m_filename != seg.m_filename || last.m_end != seg.m_begin)
		segments.push_back(seg);
	    else
		last.m_end = seg.m_end;
	}
	m_tracks[i].m_segments.swap(segments);
    }
}

void CueSheet::parseFile(const std::wstring *args)
{
    if (!m_cur_file.empty() && m_cur_file != args[1])
	this->m_has_multiple_files = true;
    m_cur_file = args[1];
}
void CueSheet::parseTrack(const std::wstring *args)
{
    if (args[2] == L"AUDIO") {
	unsigned no;
	if (std::swscanf(args[1].c_str(), L"%d", &no) != 1)
	    die("Invalid TRACK number");
	m_tracks.push_back(CueTrack(no));
    }
}
void CueSheet::parseIndex(const std::wstring *args)
{
    if (!m_tracks.size())
	die("INDEX command before TRACK");
    if (m_cur_file.empty())
	die("INDEX command before FILE");
    unsigned no, mm, ss, ff, nframes;
    if (std::swscanf(args[1].c_str(), L"%u", &no) != 1)
	die("Invalid INDEX number");
    if (std::swscanf(args[2].c_str(), L"%u:%u:%u", &mm, &ss, &ff) != 3)
	die("Invalid INDEX time format");
    if (ss > 59 || ff > 74)
	die("Invalid INDEX time format");
    nframes = msf2frames(mm, ss, ff);
    CueSegment *lastseg = lastSegment();
    if (lastseg && lastseg->m_filename == m_cur_file) {
	lastseg->m_end = nframes;
	if (lastseg->m_begin >= nframes)
	    die("INDEX time must be in ascending order");
    }
    CueSegment segment(m_cur_file, no);
    segment.m_begin = nframes;
    m_tracks.back().m_segments.push_back(segment);
}
void CueSheet::parsePostgap(const std::wstring *args)
{
    if (!m_tracks.size())
	die("POSTGAP command before TRACK");
    unsigned mm, ss, ff;
    if (std::swscanf(args[1].c_str(), L"%u:%u:%u", &mm, &ss, &ff) != 3)
	die("Invalid POSTGAP time format");
    CueSegment segment(std::wstring(L"__GAP__"), -1);
    segment.m_end = msf2frames(mm, ss, ff);
    m_tracks.back().m_segments.push_back(segment);
}
void CueSheet::parsePregap(const std::wstring *args)
{
    if (!m_tracks.size())
	die("PREGAP command before TRACK");
    unsigned mm, ss, ff;
    if (std::swscanf(args[1].c_str(), L"%u:%u:%u", &mm, &ss, &ff) != 3)
	die("Invalid PREGAP time format");
    CueSegment segment(std::wstring(L"__GAP__"), 0);
    segment.m_end = msf2frames(mm, ss, ff);
    m_tracks.back().m_segments.push_back(segment);
}
void CueSheet::parseMeta(const std::wstring *args)
{
    if (m_tracks.size())
	m_tracks.back().m_meta[args[0]] = args[1];
    else
	m_meta[args[0]] = args[1];
}

namespace Cue {
    void ConvertToItunesTags(
	    const std::map<std::wstring, std::wstring> &from,
	    std::map<uint32_t, std::wstring> *to, bool album)
    {
	std::map<std::wstring, std::wstring>::const_iterator it;
	std::map<uint32_t, std::wstring> result;
	int discnumber = 0, totaldiscs = 0;
	for (it = from.begin(); it != from.end(); ++it) {
	    std::wstring key = strutil::wslower(it->first);
	    uint32_t ikey = 0;
	    if (key == L"title")
		ikey = album ? Tag::kAlbum : Tag::kTitle;
	    else if (key == L"performer")
		ikey = album ? Tag::kAlbumArtist : Tag::kArtist;
	    else if (key == L"genre")
		ikey = Tag::kGenre;
	    else if (key == L"date")
		ikey = Tag::kDate;
	    else if (key == L"songwriter")
		ikey = Tag::kComposer;
	    else if (key == L"discnumber")
		discnumber = _wtoi(it->second.c_str());
	    else if (key == L"totaldiscs")
		totaldiscs = _wtoi(it->second.c_str());
	    if (ikey) result[ikey] = it->second;
	    if (ikey == Tag::kAlbumArtist)
		result[Tag::kArtist] = it->second;
	}
	if (discnumber) {
	    result[Tag::kDisk] =
		totaldiscs ? strutil::format(L"%d/%d", discnumber, totaldiscs)
			   : strutil::format(L"%d", discnumber);
	}
	to->swap(result);
    }

    void CueSheetToChapters(const std::wstring &cuesheet,
	    double duration,
	    std::vector<chapters::entry_t> *chapters,
	    std::map<uint32_t, std::wstring> *meta)
    {
	std::wstringbuf strbuf(cuesheet);
	CueSheet parser;
	parser.parse(&strbuf);
	if (parser.has_multiple_files())
	    throw std::runtime_error("Multiple FILE in embedded cuesheet");
	ConvertToItunesTags(parser.meta(), meta, true);

	std::vector<chapters::entry_t> chaps;
	unsigned beg, end, last_end = 0;
	for (CueSheet::iterator
	     track = parser.begin(); track != parser.end(); ++track) {
	    beg = track->begin()->m_begin;
	    end = track->begin()->m_end;
	    double track_duration;
	    if (end != -1)
		track_duration = (end - beg) / 75.0;
	    else
		track_duration = duration - (last_end / 75.0);
	    chaps.push_back(std::make_pair(track->name(), track_duration));
	    last_end = end;
	}
	chapters->swap(chaps);
    }
}
