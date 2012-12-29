#include "playlist.h"
#include "strutil.h"
#include "itunetags.h"
#include "expand.h"

namespace playlist {

    class TagLookup {
        typedef std::map<uint32_t, std::wstring> meta_t;
        const meta_t &tracktags;
    public:
        TagLookup(const meta_t &tags): tracktags(tags) {}

        std::wstring operator()(const std::wstring &name) {
            std::wstring namex = strutil::wslower(name);
            std::string skey = strutil::w2us(namex);
            uint32_t id = Vorbis::GetIDFromTagName(skey.c_str());
            if (id == 0) return L"";
            meta_t::const_iterator iter = tracktags.find(id);
            if (iter == tracktags.end())
                return L"";
            if (id != Tag::kTrack)
                return iter->second;
            int n = 0;
            std::swscanf(iter->second.c_str(), L"%d", &n);
            return strutil::format(L"%02d", n);
        }
    };

    std::wstring generateFileName(const std::wstring &spec,
                                  const std::map<uint32_t, std::wstring> &tag)
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
