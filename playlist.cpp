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
            } else 
                return strutil::us2w(iter->second);
        }
    };

    std::wstring generateFileName(const std::wstring &spec,
                                  const std::map<std::string, std::string> &tag)
    {
        std::wstring ofilename = process_template(spec, TagLookup(tag));
        struct ToSafe {
            static wchar_t call(wchar_t ch) {
                return std::wcschr(L":/\\?|<>*\"", ch) ? L'_' : ch;
            }
        };
        return strutil::strtransform(ofilename, ToSafe::call);
    }

} // namespace playlist
