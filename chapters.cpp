#include <cstdio>
#include "chapters.h"
#include "strutil.h"
#include "textfile.h"

namespace chapters {
    void add_entry(std::vector<abs_entry_t> &chapters,
                   const wchar_t *name,
                   int h, int m, double s)
    {
        std::wstring sname = name ? name : L"";
        double stamp = ((h * 60) + m) * 60 + s;
        if (!chapters.size() && stamp != 0.0)
            throw std::runtime_error("Non zero timestamp on the first chapter "
                                     "entry is not acceptable"); 
        else if (chapters.size()) {
            abs_entry_t &prev = chapters.back();
            if (prev.second >= stamp)
                throw std::runtime_error("Chapter timestamps is required to "
                                         "be strictly increasing");
        }
        chapters.push_back(std::make_pair(sname, stamp));
    }

    void load_from_file(const std::wstring &path,
                        std::vector<abs_entry_t> *chapters,
                        uint32_t codepage)
    {
        std::vector<abs_entry_t> chaps;

        std::wstring str = load_text_file(path, codepage);
        const wchar_t *tfmt = L"%02d:%02d:%lf";
        int h = 0, m = 0;
        double s = 0.0;
        strutil::Tokenizer<wchar_t> tokens(str, L"\n");
        wchar_t *tok;
        while (tok = tokens.next()) {
            if (*tok && tok[0] == L'#')
                continue;
            if (std::swscanf(tok, tfmt, &h, &m, &s) == 3) {
                strutil::strsep(&tok, L"\t ");
                add_entry(chaps, tok, h, m, s);
            } else if (wcsncmp(tok, L"Chapter", 7) == 0) {
                int hh, mm;
                double ss;
                wchar_t *key = strutil::strsep(&tok, L"=");
                if (std::wcsstr(key, L"NAME"))
                    add_entry(chaps, tok, h, m, s);
                else if (std::swscanf(tok, tfmt, &hh, &mm, &ss) == 3)
                    h = hh, m = mm, s = ss;
            }
        }
        chapters->swap(chaps);
    }

    void abs_to_duration(const std::vector<abs_entry_t> abs_ents,
                         std::vector<entry_t> *dur_ents,
                         double total_duration)
    {
        std::vector<entry_t> chapters;
        // convert from absolute timestamp to duration
        for (size_t i = 0; i < abs_ents.size(); ++i) {
            double end = (i == abs_ents.size() - 1) ? total_duration
                                                    : abs_ents[i+1].second;
            double span = end - abs_ents[i].second;
            chapters.push_back(std::make_pair(abs_ents[i].first, span));
        }
        dur_ents->swap(chapters);
    }
}

