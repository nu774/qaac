#include <sstream>
#include <clocale>
#include <locale>
#include "cuesheet.h"
#include "itunetags.h"

static inline
unsigned msf2frames(unsigned mm, unsigned ss, unsigned ff)
{
    return (mm * 60 + ss) * 75 + ff;
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
		    throw std::runtime_error(format(
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
		die(format("wrong num args for %ls command", p->cmd));
	    break;
	}
	// if (!p->cmd) die("Unknown command");
    }
    arrange();
}

void CueSheet::arrange()
{
    for (size_t i = 0; i < m_tracks.size(); ++i) {
	bool index1_found = false;
	int64_t last_index = -1;
	CueTrack &track = m_tracks[i];
	for (size_t j = 0; j < track.m_segments.size(); ++j) {
	    if (last_index >= track.m_segments[j].m_index)
		throw std::runtime_error(format("cuesheet: INDEX must be in "
						"strictly ascending order: "
						"track %u", track.m_number));
	    last_index = track.m_segments[j].m_index;
	    if (last_index == 1) index1_found = true;
	}
	if (!index1_found)
	    throw std::runtime_error(format("cuesheet: INDEX01 not found on "
					    "track %u", track.m_number));
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
	for (it = from.begin(); it != from.end(); ++it) {
	    std::wstring key = wslower(it->first);
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
	    if (ikey) result[ikey] = it->second;
	    if (ikey == Tag::kAlbumArtist)
		result[Tag::kArtist] = it->second;
	}
	to->swap(result);
    }

    void CueSheetToChapters(const std::wstring &cuesheet,
	    unsigned sample_rate, uint64_t duration,
	    std::vector<std::pair<std::wstring, int64_t> > *chapters,
	    std::map<uint32_t, std::wstring> *meta)
    {
	std::wstringbuf strbuf(cuesheet);
	CueSheet parser;
	parser.parse(&strbuf);
	if (parser.m_has_multiple_files)
	    throw std::runtime_error("Multiple FILE in embedded cuesheet");
	ConvertToItunesTags(parser.m_meta, meta, true);
	std::vector<std::pair<std::wstring, int64_t> > chaps;

	int64_t dur_acc = 0;
	for (size_t i = 0; i < parser.m_tracks.size(); ++i) {
	    CueTrack &track = parser.m_tracks[i];
	    unsigned beg = track.m_segments[0].m_begin;
	    unsigned end = track.m_segments[0].m_end;
	    int64_t dur;
	    if (end != -1) {
		dur = (double(end) - beg) * sample_rate / 75.0 + 0.5;
		dur_acc += dur;
	    }
	    else
		dur = duration - dur_acc;
	    chaps.push_back(std::make_pair(track.getName(), dur));
	}
	chapters->swap(chaps);
    }
}
