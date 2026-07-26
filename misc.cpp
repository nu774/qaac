#include "misc.h"
#include "strutil.h"
#include "win32util.h"
#include "metadata.h"
#include "expand.h"
#include <cctype>
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(pop)
#else
#include <cstdlib>
#include <cerrno>
#include <iconv.h>
#endif
#include <uchardet.h>
#include <regex>

namespace misc
{
#ifdef _WIN32
    int getCodePageFromCharset(const char* charset)
    {
        std::cmatch match;
        std::regex windowscp("(?:IBM|CP|WINDOWS)-?(\\d+)");
        std::regex iso8859("ISO-8859-(\\d+)");
        std::regex iso2022("ISO-2022-(\\w+)");
        std::regex euc("EUC-(\\w+)");
        std::regex mac("MAC-(\\w+)");

        if (!charset)
            return -1;
        if (std::strcmp(charset, "ASCII") == 0)
            return 20127;
        if (std::strcmp(charset, "UTF-8") == 0)
            return 65001;
        else if (std::strcmp(charset, "UTF-16") == 0)
            return 0;
        else if (std::strcmp(charset, "SHIFT_JIS") == 0)
            return 932;
        else if (std::strcmp(charset, "BIG5") == 0)
            return 950;
        else if (std::strcmp(charset, "KOI8-R") == 0)
            return 20866;
        else if (std::strcmp(charset, "HZ-GB-2312") == 0)
            return 52936;
        else if (std::strcmp(charset, "GB18030") == 0)
            return 54936;
        else if (std::regex_match(charset, match, windowscp))
            return atoi(match[1].str().c_str());
        else if (std::regex_match(charset, match, iso8859))
            return 28590 + atoi(match[1].str().c_str());
        else if (std::regex_match(charset, match, iso2022)) {
            if (match[1].str() == "JP")
                return 50220;
            if (match[1].str() == "KR")
                return 50225;
            if (match[1].str() == "CN")
                return 50227;
        }
        else if (std::regex_match(charset, match, euc)) {
            if (match[1].str() == "JP")
                return 51932;
            if (match[1].str() == "KR")
                return 51949;
            if (match[1].str() == "TW")
                return 51950;
        }
        else if (std::regex_match(charset, match, mac)) {
            if (match[1].str() == "CYRILLIC")
                return 10007;
            if (match[1].str() == "CENTRALEUROPE")
                return 10029;
        }
        return -1;
    }
#else
    std::string codepageToIconvName(int codepage)
    {
        switch (codepage) {
        case 0:     return "UTF-16";
        case 20127: return "ASCII";
        case 65001: return "UTF-8";
        case 932:   return "SHIFT_JIS";
        case 950:   return "BIG5";
        case 20866: return "KOI8-R";
        case 52936: return "HZ-GB-2312";
        case 54936: return "GB18030";
        case 50220: return "ISO-2022-JP";
        case 50225: return "ISO-2022-KR";
        case 50227: return "ISO-2022-CN";
        case 51932: return "EUC-JP";
        case 51949: return "EUC-KR";
        case 51950: return "EUC-TW";
        case 10007: return "MAC-CYRILLIC";
        case 10029: return "MAC-CENTRALEUROPE";
        }
        if (codepage >= 28591 && codepage <= 28599)
            return strutil::format("ISO-8859-%d", codepage - 28590);
        return strutil::format("CP%d", codepage);
    }

    std::string iconv_to_utf8(const char *charset, const char *data, size_t len)
    {
        iconv_t cd = iconv_open("UTF-8", charset);
        if (cd == (iconv_t)-1)
            throw std::runtime_error(std::string("unsupported charset: ") + charset);
        std::shared_ptr<void> cd_closer(0, [cd](void*) { iconv_close(cd); });
        std::string result;
        std::vector<char> outbuf(len * 4 + 16);
        char *inp = const_cast<char*>(data);
        size_t inleft = len;
        char *outp = outbuf.data();
        size_t outleft = outbuf.size();
        while (inleft > 0) {
            size_t rc = iconv(cd, &inp, &inleft, &outp, &outleft);
            if (rc == (size_t)-1) {
                if (errno == E2BIG) {
                    size_t used = outbuf.size() - outleft;
                    outbuf.resize(outbuf.size() * 2);
                    outp = outbuf.data() + used;
                    outleft = outbuf.size() - used;
                    continue;
                }
                throw std::runtime_error(
                    std::string("charset conversion failed (") + charset + ")");
            }
        }
        result.assign(outbuf.data(), outbuf.size() - outleft);
        return result;
    }
#endif

    std::string loadTextFile(const std::string &path, int codepage)
    {
        auto fp = std::shared_ptr<FILE>(win32::wfopenx(path, "rb"), std::fclose);
        fseeko(fp.get(), 0, SEEK_END);
        int64_t fileSize = ftello(fp.get());
        fseeko(fp.get(), 0, SEEK_SET);
        if (fileSize > 0x100000) {
            throw std::runtime_error(path + ": file too big");
        }
        std::vector<char> ibuf(fileSize);
        int n = std::fread(ibuf.data(), 1, fileSize, fp.get());
        ibuf.resize(n);
#ifdef _WIN32
        if (!codepage) {
            auto detector = std::shared_ptr<uchardet>(uchardet_new(), uchardet_delete);
            if (uchardet_handle_data(detector.get(), ibuf.data(), ibuf.size())) {
                throw std::runtime_error(path + ": uchardet_handle_data() failed");
            }
            uchardet_data_end(detector.get());
            auto charset = uchardet_get_charset(detector.get());
            if (!charset)
                throw std::runtime_error(path + ": cannot detect code page");
            codepage = getCodePageFromCharset(charset);
            if (codepage < 0)
                throw std::runtime_error(path + ": unknown charset");
        }
        std::vector<wchar_t> obuf;
        if (codepage == 0) {
            obuf.resize(ibuf.size() / sizeof(wchar_t));
            std::memcpy(obuf.data(), ibuf.data(), ibuf.size());
        } else {
            int nc = MultiByteToWideChar(codepage, 0, ibuf.data(), ibuf.size(), nullptr, 0);
            obuf.resize(nc);
            MultiByteToWideChar(codepage, 0, ibuf.data(), ibuf.size(), obuf.data(), obuf.size());
        }
        obuf.push_back(0);
        // chop off BOM
        size_t bom = (obuf.size() && obuf[0] == 0xfeff) ? 1 : 0;
        return strutil::w2us(strutil::normalize_crlf(&obuf[bom], L"\n"));
#else
        std::string charset;
        if (codepage)
            charset = codepageToIconvName(codepage);
        else {
            auto detector = std::shared_ptr<uchardet>(uchardet_new(), uchardet_delete);
            if (uchardet_handle_data(detector.get(), ibuf.data(), ibuf.size())) {
                throw std::runtime_error(path + ": uchardet_handle_data() failed");
            }
            uchardet_data_end(detector.get());
            auto detected = uchardet_get_charset(detector.get());
            if (!detected || !*detected)
                throw std::runtime_error(path + ": cannot detect code page");
            charset = detected;
        }
        std::string decoded = iconv_to_utf8(charset.c_str(), ibuf.data(), ibuf.size());
        // chop off BOM
        if (decoded.size() >= 3 &&
            (unsigned char)decoded[0] == 0xef &&
            (unsigned char)decoded[1] == 0xbb &&
            (unsigned char)decoded[2] == 0xbf)
            decoded.erase(0, 3);
        return strutil::normalize_crlf(decoded.c_str(), "\n");
#endif
    }

    class TagLookup {
        typedef std::map<std::string, std::string> meta_t;
        const meta_t &tracktags;
    public:
        TagLookup(const meta_t &tags): tracktags(tags) {}

        std::string operator()(const std::string &name) {
            std::string key = TextBasedTag::normalizeTagName(name.c_str());
            meta_t::const_iterator iter = tracktags.find(key);
            if (iter == tracktags.end())
                return "";
            else if (key == "track number" || key == "DISC NUMBER") {
                strutil::Tokenizer<char> tok(iter->second, "/");
                unsigned n = 0;
                sscanf(tok.next(), "%u", &n);
                return strutil::format("%02u", n);
            }
            return strutil::strtransform(iter->second, [](char c)->char {
                return std::strchr(":/\\?|<>*\"", c) ? '_' : c;
            });
        }
    };

    std::string generateFileName(const std::string &spec,
                                 const std::map<std::string, std::string> &tag)
    {
        auto spec2 = strutil::strtransform(spec, [](char c)->char {
                                           return c == '\\' ? '/' : c;
                                           });
        auto res = process_template(spec2, TagLookup(tag));
        std::vector<std::string> comp;
        strutil::Tokenizer<char> tokens(res, "/");
        char *tok;
        while ((tok = tokens.next())) {
            std::string t(tok);
            size_t b = t.find_first_not_of(" \t");
            t = (b == std::string::npos) ? ""
                : t.substr(b, t.find_last_not_of(" \t") - b + 1);
            if (t.size() > 250) {
                t.resize(250);
                size_t i = t.size();
                while (i > 0 && (static_cast<unsigned char>(t[i-1]) & 0xC0) == 0x80)
                    --i;
                if (i > 0) {
                    unsigned char lead = static_cast<unsigned char>(t[i-1]);
                    size_t seqlen = (lead & 0x80) == 0 ? 1
                                  : (lead & 0xE0) == 0xC0 ? 2
                                  : (lead & 0xF0) == 0xE0 ? 3
                                  : (lead & 0xF8) == 0xF0 ? 4 : 1;
                    if (i - 1 + seqlen > t.size())
                        --i;
                }
                t.resize(i);
            }
            comp.push_back(t);
        }
        res.clear();
        for (size_t i = 0; i < comp.size() - 1; ++i)
            res += comp[i] + "/";
        res += comp[comp.size() - 1];
        return res;
    }

    void add_chapter_entry(std::vector<chapter_t> &chapters,
                           const char *name,
                           int h, int m, double s)
    {
        std::string sname = name ? name : "";
        double stamp = ((h * 60) + m) * 60 + s;
        if (!chapters.size() && stamp != 0.0)
            throw std::runtime_error("Non zero timestamp on the first chapter "
                                     "entry is not allowed");
        else if (chapters.size()) {
            chapter_t &prev = chapters.back();
            if (prev.second >= stamp)
                throw std::runtime_error("Chapter timestamps is required to "
                                         "be strictly increasing");
        }
        chapters.push_back(std::make_pair(sname, stamp));
    }

    std::vector<chapter_t> loadChapterFile(const char *path,
                                           uint32_t codepage)
    {
        std::vector<chapter_t> chaps;

        std::string str = misc::loadTextFile(path, codepage);
        const char *tfmt = "%02d:%02d:%lf";
        int h = 0, m = 0;
        double s = 0.0;
        strutil::Tokenizer<char> tokens(str, "\n");
        char *tok;
        while ((tok = tokens.next())) {
            if (*tok && tok[0] == '#')
                continue;
            if (std::sscanf(tok, tfmt, &h, &m, &s) == 3) {
                strutil::strsep(&tok, "\t ");
                add_chapter_entry(chaps, tok, h, m, s);
            } else if (strncmp(tok, "Chapter", 7) == 0) {
                int hh, mm;
                double ss;
                char *key = strutil::strsep(&tok, "=");
                if (std::strstr(key, "NAME"))
                    add_chapter_entry(chaps, tok, h, m, s);
                else if (std::sscanf(tok, tfmt, &hh, &mm, &ss) == 3)
                    h = hh, m = mm, s = ss;
            }
        }
        return chaps;
    }

    // converts absolute timestamp to time delta
    std::vector<chapter_t>
        convertChaptersToQT(const std::vector<chapter_t> &chapters,
                            double total_duration)
    {
        std::vector<chapter_t> result;
        auto first = chapters.begin();
        auto last  = chapters.end();
        if (first != last) {
            auto prev_name = first->first;
            auto prev_stamp = first->second;
            for (auto it = ++first; it != last; ++it) {
                double delta = it->second - prev_stamp;
                result.push_back(std::make_pair(prev_name, delta));
                prev_name = it->first;
                prev_stamp = it->second;
            }
            double last_delta = total_duration - prev_stamp;
            result.push_back(std::make_pair(prev_name, last_delta));
        }
        return result;
    }
    std::shared_ptr<FILE> openConfigFile(const char *file)
    {
        std::vector<std::string> search_paths;
#ifdef _WIN32
        const wchar_t *home = _wgetenv(L"HOME");
        if (home)
            search_paths.push_back(
                strutil::format("%s\\%s", strutil::w2us(home).c_str(), ".qaac"));
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, path)))
            search_paths.push_back(
                strutil::format("%s\\%s", strutil::w2us(path).c_str(), "qaac"));
        search_paths.push_back(win32::get_module_directory());
        const char *sep = "\\";
#else
        const char *home = getenv("HOME");
        if (home)
            search_paths.push_back(strutil::format("%s/%s", home, ".qaac"));
        const char *xdg = getenv("XDG_CONFIG_HOME");
        if (xdg)
            search_paths.push_back(strutil::format("%s/qaac", xdg));
        else if (home)
            search_paths.push_back(strutil::format("%s/.config/qaac", home));
        search_paths.push_back(win32::get_module_directory());
        const char *sep = "/";
#endif
        for (size_t i = 0; i < search_paths.size(); ++i) {
            try {
                std::string pathtry =
                    strutil::format("%s%s%s", search_paths[i].c_str(), sep, file);
                return win32::fopen(pathtry, "r");
            } catch (...) {
                if (i == search_paths.size() - 1) throw;
            }
        }
        return 0;
    }

    static
    std::vector<std::vector<complex_t>>
    loadRemixerMatrix(std::shared_ptr<FILE> fileptr)
    {
        FILE *fp = fileptr.get();
        int c;
        std::vector<std::vector<complex_t> > matrix;
        std::vector<complex_t> row;
        while ((c = std::getc(fp)) != EOF) {
            if (c == '\n') {
                if (row.size()) {
                    matrix.push_back(row);
                    row.clear();
                }
            } else if (std::isspace(c)) {
                while (c != '\n' && std::isspace(c = std::getc(fp)))
                    ;
                std::ungetc(c, fp);
            } else if (std::isdigit(c) || c == '-') {
                std::ungetc(c, fp);
                double v;
                if (std::fscanf(fp, "%lf", &v) != 1)
                    throw std::runtime_error("invalid matrix preset file");
                c = std::getc(fp);
                if (std::strchr("iIjJ", c))
                    row.push_back(complex_t(0.0, v));
                else if (std::strchr("kK", c))
                    row.push_back(complex_t(0.0, -v));
                else {
                    std::ungetc(c, fp);
                    row.push_back(complex_t(v, 0.0));
                }
            } else
                throw std::runtime_error("invalid char in matrix preset file");
        }
        if (row.size())
            matrix.push_back(row);
        return matrix;
    }

    std::vector<std::vector<complex_t>>
    loadRemixerMatrixFromFile(const char *path)
    {
        return loadRemixerMatrix(win32::fopen(path, "r"));
    }

    std::vector<std::vector<complex_t>>
    loadRemixerMatrixFromPreset(const char *preset_name)
    {
#ifdef _WIN32
        std::string path = strutil::format("matrix\\%s.txt", preset_name);
#else
        std::string path = strutil::format("matrix/%s.txt", preset_name);
#endif
        return loadRemixerMatrix(openConfigFile(path.c_str()));
    }

}
