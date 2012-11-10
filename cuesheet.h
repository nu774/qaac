#include <streambuf>
#include <istream>
#include <string>
#include <vector>
#include <map>
#include "util.h"
#include "iointer.h"

template <typename CharT>
struct CueTokenizer {
    typedef std::char_traits<CharT> traits_type;
    typedef typename traits_type::int_type int_type;

    explicit CueTokenizer(std::basic_streambuf<CharT> *sb)
	: m_sb(sb), m_lineno(0)
    {}
    bool isWS(int_type c) { return c == ' ' || c == '\t' || c == '\r'; }
    bool nextline();

    std::basic_streambuf<CharT> *m_sb;
    std::vector<std::basic_string<CharT> > m_fields;
    size_t m_lineno;
};

struct CueSegment {
    CueSegment(const std::wstring &filename, unsigned index)
	: m_filename(filename), m_index(index), m_begin(0), m_end(-1)
    {}
    std::wstring m_filename;
    unsigned m_index;
    unsigned m_begin;
    unsigned m_end;
};

class CueTrack {
    friend class CueSheet;
    unsigned m_number;
    std::vector<CueSegment> m_segments;
    std::map<std::wstring, std::wstring> m_meta;
public:
    typedef std::vector<CueSegment>::const_iterator iterator;

    CueTrack(unsigned number) : m_number(number) {}
    std::wstring name() const
    {
	std::map<std::wstring, std::wstring>::const_iterator
	    it = m_meta.find(L"TITLE");
	return it == m_meta.end() ? L"" : it->second;
    }
    unsigned number() const { return m_number; }
    const std::map<std::wstring, std::wstring> &meta() const
    {
	return m_meta;
    }
    iterator begin() const { return m_segments.begin(); }
    iterator end() const { return m_segments.end(); }
};

class CueSheet {
    bool m_has_multiple_files;
    size_t m_lineno;
    std::wstring m_cur_file;
    std::vector<CueTrack> m_tracks;
    std::map<std::wstring, std::wstring> m_meta;
public:
    typedef std::vector<CueTrack>::const_iterator iterator;

    CueSheet(): m_has_multiple_files(false) {}
    void parse(std::wstreambuf *src);
    void loadTracks(std::vector<chapters::Track> &tracks,
		    const std::wstring &cuedir,
		    const std::wstring &fname_format);

    unsigned count() const { return m_tracks.size(); }
    const std::map<std::wstring, std::wstring> &meta() const
    {
	return m_meta;
    }
    iterator begin() const { return m_tracks.begin(); }
    iterator end() const { return m_tracks.end(); }
    bool has_multiple_files() const { return m_has_multiple_files; }
private:
    void arrange();
    void parseFile(const std::wstring *args);
    void parseTrack(const std::wstring *args);
    void parseIndex(const std::wstring *args);
    void parsePostgap(const std::wstring *args);
    void parsePregap(const std::wstring *args);
    void parseMeta(const std::wstring *args);
    void parseRem(const std::wstring *args) { parseMeta(args + 1); }
    void die(const std::string &msg)
    {
	throw std::runtime_error(strutil::format("cuesheet: %s at line %d",
						 msg.c_str(), m_lineno));
    }
    CueSegment *lastSegment()
    {
	for (ssize_t i = m_tracks.size() - 1; i >= 0; --i)
	    if (m_tracks[i].m_segments.size())
		return &m_tracks[i].m_segments.back();
	return 0;
    }
};

namespace Cue {
    void ConvertToItunesTags(const std::map<std::wstring, std::wstring> &from,
	std::map<uint32_t, std::wstring> *to, bool album=false);

    void CueSheetToChapters(const std::wstring &cuesheet,
	double duration,
	std::vector<chapters::entry_t> *chapters,
	std::map<uint32_t, std::wstring> *meta);
}
