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
#include "playlist.h"

static inline
unsigned msf2frames(unsigned mm, unsigned ss, unsigned ff)
{
    return (mm * 60 + ss) * 75 + ff;
}

static inline
uint64_t frame2sample(double sampling_rate, uint32_t nframe)
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
    return m_fields.size() > 0 || c == '\n';
}

template struct CueTokenizer<char>;
template struct CueTokenizer<wchar_t>;

void CueTrack::addSegment(const CueSegment &seg)
{
    if (m_segments.size()) {
        CueSegment &last = m_segments.back();
        if (last.m_index >= seg.m_index) {
            if (last.m_index == 0x7fffffff) {
                std::string msg =
                    strutil::format("cuesheet: conflicting use of "
                                    "INDEX00 or PREGAP found on "
                                    "track %u-%u", m_number, m_number + 1);
                throw std::runtime_error(msg);
            }
            std::string msg =
                strutil::format("cuesheet: INDEX must be in strictly "
                                "ascending order: track %u", m_number);
            throw std::runtime_error(msg);
        }
        if (last.m_filename == seg.m_filename && last.m_end == seg.m_begin)
        {
            last.m_end = seg.m_end;
            return;
        }
    }
    m_segments.push_back(seg);
}

void CueTrack::iTunesTags(std::map<uint32_t, std::wstring> *tags) const
{
    std::map<uint32_t, std::wstring> result;
    m_cuesheet->iTunesTags(&result);
    std::map<std::wstring, std::wstring>::const_iterator it;
    for (it = m_meta.begin(); it != m_meta.end(); ++it) {
        std::wstring key = strutil::wslower(it->first);
        uint32_t ikey = 0;
        if (key == L"title")
            ikey = Tag::kTitle;
        else if (key == L"performer")
            ikey = Tag::kArtist;
        else if (key == L"genre")
            ikey = Tag::kGenre;
        else if (key == L"date")
            ikey = Tag::kDate;
        else if (key == L"songwriter")
            ikey = Tag::kComposer;
        if (ikey) result[ikey] = it->second;
    }
    CueSheet::const_iterator last = m_cuesheet->end() - 1;
    result[Tag::kTrack] =
        strutil::format(L"%d/%d", number(), last->number());
    std::map<uint32_t, std::wstring>::iterator iaa
        = result.find(Tag::kAlbumArtist);
    if (result.find(Tag::kArtist) == result.end() && iaa != result.end())
        result[Tag::kArtist] = iaa->second;
    tags->swap(result);
}

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
    validate();
}

void CueSheet::loadTracks(playlist::Playlist &tracks,
                          const std::wstring &cuedir,
                          const std::wstring &fname_format)
{
    std::shared_ptr<ISeekableSource> src;
    for (const_iterator track = begin(); track != end(); ++track) {
        std::shared_ptr<CompositeSource> track_source(new CompositeSource());
        std::map<uint32_t, std::wstring> track_tags;
        track->iTunesTags(&track_tags);
        track_source->setTags(track_tags);
        for (CueTrack::const_iterator
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
            double rate = src->getSampleFormat().mSampleRate;
            uint64_t begin = frame2sample(rate, segment->m_begin);
            uint64_t duration = ~0ULL;
            if (segment->m_end != ~0U)
                duration = frame2sample(rate, segment->m_end) - begin;
            src.reset(new TrimmedSource(src, begin, duration));
            track_source->addSource(src);
        }
        std::wstring ofilename =
            playlist::generateFileName(fname_format, track_tags) + L".stub";

        playlist::Track new_track;
        new_track.name = track->name();
        new_track.source = track_source;
        new_track.ofilename = ofilename;
        tracks.push_back(new_track);
    }
}

void CueSheet::asChapters(double duration,
                          std::vector<chapters::entry_t> *chapters) const
{
    if (m_has_multiple_files)
        throw std::runtime_error("Multiple FILE in embedded cuesheet");

    std::vector<chapters::entry_t> chaps;
    unsigned tbeg, tend, last_end = 0;
    for (const_iterator track = begin(); track != end(); ++track) {
        tbeg = track->begin()->m_begin;
        tend = track->begin()->m_end;
        double track_duration;
        if (tend != ~0U)
            track_duration = (tend - tbeg) / 75.0;
        else
            track_duration = duration - (last_end / 75.0);
        std::wstring title = track->name();
        if (title == L"")
            title = strutil::format(L"Track %02d", track->number()); 
        chaps.push_back(std::make_pair(title, track_duration));
        last_end = tend;
    }
    chapters->swap(chaps);
}

void CueSheet::iTunesTags(std::map<uint32_t, std::wstring> *tags) const
{
    std::map<std::wstring, std::wstring>::const_iterator it;
    std::map<uint32_t, std::wstring> result;
    int discnumber = 0, totaldiscs = 0;
    for (it = m_meta.begin(); it != m_meta.end(); ++it) {
        std::wstring key = strutil::wslower(it->first);
        uint32_t ikey = 0;
        if (key == L"title")
            ikey = Tag::kAlbum;
        else if (key == L"performer")
            ikey = Tag::kAlbumArtist;
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
    }
    if (discnumber) {
        result[Tag::kDisk] =
            totaldiscs ? strutil::format(L"%d/%d", discnumber, totaldiscs)
                       : strutil::format(L"%d", discnumber);
    }
    tags->swap(result);
}

void CueSheet::validate()
{
    CueTrack::const_iterator segment;
    for (iterator track = begin(); track != end(); ++track) {
        bool index1_found = false;
        for (segment = track->begin(); segment != track->end(); ++segment)
            if (segment->m_index == 1)
                index1_found = true;
        if (!index1_found) {
            std::string msg =
                strutil::format("cuesheet: INDEX01 not found on track %u",
                                track->number());
            throw std::runtime_error(msg);
        }
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
        m_tracks.push_back(CueTrack(this, no));
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
    if (no > 0)
        m_tracks.back().addSegment(segment);
    else {
        if (m_tracks.size() == 1) {
            /* HTOA */
            m_tracks.insert(m_tracks.begin(), CueTrack(this, 0));
            m_tracks[0].setTag(L"title", L"(HTOA)");
            segment.m_index = 1;
        } else
            segment.m_index = 0x7fffffff;
        m_tracks[m_tracks.size() - 2].addSegment(segment);
    }
}
void CueSheet::parsePostgap(const std::wstring *args)
{
    if (!m_tracks.size())
        die("POSTGAP command before TRACK");
    unsigned mm, ss, ff;
    if (std::swscanf(args[1].c_str(), L"%u:%u:%u", &mm, &ss, &ff) != 3)
        die("Invalid POSTGAP time format");
    CueSegment segment(std::wstring(L"__GAP__"), 0x7ffffffe);
    segment.m_end = msf2frames(mm, ss, ff);
    m_tracks.back().addSegment(segment);
}
void CueSheet::parsePregap(const std::wstring *args)
{
    if (!m_tracks.size())
        die("PREGAP command before TRACK");
    unsigned mm, ss, ff;
    if (std::swscanf(args[1].c_str(), L"%u:%u:%u", &mm, &ss, &ff) != 3)
        die("Invalid PREGAP time format");
    CueSegment segment(std::wstring(L"__GAP__"), 0x7fffffff);
    segment.m_end = msf2frames(mm, ss, ff);
    if (m_tracks.size() > 1)
        m_tracks[m_tracks.size() - 2].addSegment(segment);
}
void CueSheet::parseMeta(const std::wstring *args)
{
    if (m_tracks.size())
        m_tracks.back().setTag(args[0], args[1]);
    else
        m_meta[args[0]] = args[1];
}

namespace Cue {
    void CueSheetToChapters(const std::wstring &cuesheet,
            double duration,
            std::vector<chapters::entry_t> *chapters,
            std::map<uint32_t, std::wstring> *meta)
    {
        std::wstringbuf strbuf(cuesheet);
        CueSheet parser;
        parser.parse(&strbuf);
        parser.iTunesTags(meta);
        if (meta->find(Tag::kAlbumArtist) != meta->end())
            (*meta)[Tag::kArtist] = (*meta)[Tag::kAlbumArtist];
        parser.asChapters(duration, chapters);
    }
}
