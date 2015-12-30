#include "playlist.h"
#include "strutil.h"
#include "metadata.h"
#include "expand.h"

namespace playlist {

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
        while (tok = tokens.next()) {
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

} // namespace playlist
