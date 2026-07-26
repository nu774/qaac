#include <memory>
#include <streambuf>
#include <istream>
#include <string>
#include <vector>
#include <map>
#include "util.h"
#include "ISource.h"

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
    CueSegment(const std::string &filename, unsigned index)
        : m_filename(filename), m_index(index), m_begin(0), m_end(~0U)
    {}
    std::string m_filename;
    unsigned m_index;
    unsigned m_begin;
    unsigned m_end;
};

class CueSheet;

class CueTrack {
    CueSheet *m_cuesheet;
    unsigned m_number;
    std::vector<CueSegment> m_segments;
    std::map<std::string, std::string> m_meta;
public:
    typedef std::vector<CueSegment>::iterator iterator;
    typedef std::vector<CueSegment>::const_iterator const_iterator;

    CueTrack(CueSheet *cuesheet, unsigned number)
        : m_cuesheet(cuesheet), m_number(number) {}
    std::string name() const
    {
        std::map<std::string, std::string>::const_iterator
            it = m_meta.find("TITLE");
        return it == m_meta.end() ? "" : it->second;
    }
    unsigned number() const { return m_number; }
    void addSegment(const CueSegment &seg);
    void setMeta(const std::string &key, const std::string &value)
    {
        m_meta[key] = value;
    }
    std::map<std::string, std::string> getTags() const;

    iterator begin() { return m_segments.begin(); }
    iterator end() { return m_segments.end(); }
    const_iterator begin() const { return m_segments.begin(); }
    const_iterator end() const { return m_segments.end(); }
    CueSegment *lastSegment()
    {
        return m_segments.size() ? &m_segments.back() : 0;
    }
};

class CueSheet {
    bool m_has_multiple_files;
    size_t m_lineno;
    std::string m_cur_file;
    std::vector<CueTrack> m_tracks;
    std::map<std::string, std::string> m_meta;
public:
    typedef std::vector<CueTrack>::iterator iterator;
    typedef std::vector<CueTrack>::const_iterator const_iterator;

    CueSheet(): m_has_multiple_files(false) {}
    void parse(std::streambuf *src);
    std::vector<std::shared_ptr<ISeekableSource>>
        loadTracks(bool is_embedded,
                   const std::string &cue_or_container_path,
                   const std::vector<int> &selection);
    std::map<std::string, std::string> getTags() const;

    unsigned count() const { return m_tracks.size(); }
    iterator begin() { return m_tracks.begin(); }
    iterator end() { return m_tracks.end(); }
    const_iterator begin() const { return m_tracks.begin(); }
    const_iterator end() const { return m_tracks.end(); }
private:
    void validate();
    void parseFile(const std::string *args);
    void parseTrack(const std::string *args);
    void parseIndex(const std::string *args);
    void parsePostgap(const std::string *args);
    void parsePregap(const std::string *args);
    void parseMeta(const std::string *args);
    void parseRem(const std::string *args) { parseMeta(args + 1); }
    void die(const std::string &msg)
    {
        throw std::runtime_error(strutil::format("cuesheet: %s at line %d",
                                                 msg.c_str(), m_lineno));
    }
    CueSegment *lastSegment()
    {
        CueSegment *seg;
        for (ssize_t i = m_tracks.size() - 1; i >= 0; --i)
            if ((seg = m_tracks[i].lastSegment()) != 0)
                return seg;
        return 0;
    }
};
