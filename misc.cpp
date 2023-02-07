#include "misc.h"
#include "strutil.h"
#include "win32util.h"
#include "metadata.h"
#include "expand.h"
#include <cctype>
#pragma warning(push)
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(pop)
#include <mlang.h>
#include <uchardet.h>
#include <regex>

namespace misc
{
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

    std::wstring loadTextFile(const std::wstring &path, int codepage)
    {
        auto fp = std::shared_ptr<FILE>(win32::wfopenx(path.c_str(), L"rb"), std::fclose);
        int64_t fileSize = _filelengthi64(fileno(fp.get()));
        if (fileSize > 0x100000) {
            throw std::runtime_error(strutil::w2us(path + L": file too big"));
        }
        std::vector<char> ibuf(fileSize);
        int n = std::fread(ibuf.data(), 1, fileSize, fp.get());
        ibuf.resize(n);
        if (!codepage) {
            auto detector = std::shared_ptr<uchardet>(uchardet_new(), uchardet_delete);
            if (uchardet_handle_data(detector.get(), ibuf.data(), ibuf.size())) {
                throw std::runtime_error(strutil::w2us(path + L": uchardet_handle_data() failed"));
            }
            uchardet_data_end(detector.get());
            auto charset = uchardet_get_charset(detector.get());
            if (charset < 0)
                throw std::runtime_error(strutil::w2us(path + L": cannot detect code page"));
            codepage = getCodePageFromCharset(charset);
            if (codepage < 0)
                throw std::runtime_error(strutil::w2us(path + L": unknown charset"));
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
        return strutil::normalize_crlf(&obuf[bom], L"\n");
    }

    class TagLookup {
        typedef std::map<std::string, std::string> meta_t;
        const meta_t &tracktags;
    public:
        TagLookup(const meta_t &tags): tracktags(tags) {}

        std::wstring operator()(const std::wstring &name) {
            std::string key =
                TextBasedTag::normalizeTagName(strutil::w2us(name).c_str());
            meta_t::const_iterator iter = tracktags.find(key);
            if (iter == tracktags.end())
                return L"";
            else if (key == "track number" || key == "DISC NUMBER") {
                strutil::Tokenizer<char> tok(iter->second, "/");
                unsigned n = 0;
                sscanf(tok.next(), "%u", &n);
                return strutil::format(L"%02u", n);
            }
            auto val = strutil::us2w(iter->second);
            return strutil::strtransform(val, [](wchar_t c)->wchar_t {
                return std::wcschr(L":/\\?|<>*\"", c) ? L'_' : c;
            });
        }
    };

    std::wstring generateFileName(const std::wstring &spec,
                                  const std::map<std::string, std::string> &tag)
    {
        auto spec2 = strutil::strtransform(spec, [](wchar_t c)->wchar_t {
                                           return c == L'\\' ? L'/' : c;
                                           });
        auto res = process_template(spec2, TagLookup(tag));
        std::vector<std::wstring> comp;
        strutil::Tokenizer<wchar_t> tokens(res, L"/");
        wchar_t *tok;
        while ((tok = tokens.next())) {
            if (wcslen(tok) > 250)
                tok[250] = 0;
            comp.push_back(tok);
        }
        res.clear();
        for (size_t i = 0; i < comp.size() - 1; ++i)
            res += comp[i] + L"/";
        res += comp[comp.size() - 1];
        return res;
    }

    void add_chapter_entry(std::vector<chapter_t> &chapters,
                           const wchar_t *name,
                           int h, int m, double s)
    {
        std::wstring sname = name ? name : L"";
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

    std::vector<chapter_t> loadChapterFile(const wchar_t *path,
                                           uint32_t codepage)
    {
        std::vector<chapter_t> chaps;

        std::wstring str = misc::loadTextFile(path, codepage);
        const wchar_t *tfmt = L"%02d:%02d:%lf";
        int h = 0, m = 0;
        double s = 0.0;
        strutil::Tokenizer<wchar_t> tokens(str, L"\n");
        wchar_t *tok;
        while ((tok = tokens.next())) {
            if (*tok && tok[0] == L'#')
                continue;
            if (std::swscanf(tok, tfmt, &h, &m, &s) == 3) {
                strutil::strsep(&tok, L"\t ");
                add_chapter_entry(chaps, tok, h, m, s);
            } else if (wcsncmp(tok, L"Chapter", 7) == 0) {
                int hh, mm;
                double ss;
                wchar_t *key = strutil::strsep(&tok, L"=");
                if (std::wcsstr(key, L"NAME"))
                    add_chapter_entry(chaps, tok, h, m, s);
                else if (std::swscanf(tok, tfmt, &hh, &mm, &ss) == 3)
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
    std::shared_ptr<FILE> openConfigFile(const wchar_t *file)
    {
        std::vector<std::wstring> search_paths;
        const wchar_t *home = _wgetenv(L"HOME");
        if (home)
            search_paths.push_back(strutil::format(L"%s\\%s", home, L".qaac"));
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, path)))
            search_paths.push_back(strutil::format(L"%s\\%s", path, L"qaac"));
        search_paths.push_back(win32::get_module_directory());
        for (size_t i = 0; i < search_paths.size(); ++i) {
            try {
                std::wstring pathtry =
                    strutil::format(L"%s\\%s", search_paths[i].c_str(), file);
                return win32::fopen(pathtry, L"r");
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
    loadRemixerMatrixFromFile(const wchar_t *path)
    {
        return loadRemixerMatrix(win32::fopen(path, L"r"));
    }

    std::vector<std::vector<complex_t>>
    loadRemixerMatrixFromPreset(const wchar_t *preset_name)
    {
        std::wstring path = strutil::format(L"matrix\\%s.txt", preset_name);
        return loadRemixerMatrix(openConfigFile(path.c_str()));
    }

}
