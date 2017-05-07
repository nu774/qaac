#include "misc.h"
#include "strutil.h"
#include "win32util.h"
#include "metadata.h"
#include "expand.h"
#include <cctype>
#include <shlobj.h>
#include <mlang.h>

namespace misc
{
    static inline void throwIfError(HRESULT expr, const char *msg)
    {
        if (FAILED(expr))
            win32::throw_error(msg, expr);
    }
#define HR(expr) (void)(throwIfError((expr), #expr))

    std::wstring loadTextFile(const std::wstring &path, uint32_t codepage)
    {
        IStream *stream;
        HRESULT hr = SHCreateStreamOnFileW(path.c_str(),
                                           STGM_READ | STGM_SHARE_DENY_WRITE,
                                           &stream);
        if (FAILED(hr)) win32::throw_error(path, hr);
        auto release_func = [](IUnknown *x) { x->Release(); };
        std::shared_ptr<IStream> streamPtr(stream, release_func);

        LARGE_INTEGER li = {{ 0 }};
        ULARGE_INTEGER ui;
        HR(stream->Seek(li, STREAM_SEEK_END, &ui));
        if (!ui.QuadPart)
            return L"";
        else if (ui.QuadPart > 0x100000) {
            throw std::runtime_error(strutil::w2us(path + L": file too big"));
        }
        size_t fileSize = ui.LowPart;
        HR(stream->Seek(li, STREAM_SEEK_SET, &ui));

        IMultiLanguage2 *mlang;
        HR(CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_INPROC_SERVER,
                    IID_IMultiLanguage2, (void**)(&mlang)));
        std::shared_ptr<IMultiLanguage2> mlangPtr(mlang, release_func);

        if (!codepage) {
            DetectEncodingInfo encoding[5];
            INT nscores = 5;
            HR(mlang->DetectCodepageInIStream(0, GetACP(),
                                              stream, encoding, &nscores));
            codepage = encoding[0].nCodePage;
            for (size_t i = 0; i < nscores; ++i)
                if (encoding[i].nCodePage == 65001) {
                    codepage = 65001;
                    break;
                }
            HR(stream->Seek(li, STREAM_SEEK_SET, &ui));
        }
        std::vector<char> ibuf(fileSize);
        ULONG nread;
        HR(stream->Read(&ibuf[0], ibuf.size(), &nread));
        if (fileSize >= 3 && std::memcmp(&ibuf[0], "\xef\xbb\xbf", 3) == 0)
            codepage = 65001; // UTF-8
        else if (fileSize >= 2 && std::memcmp(&ibuf[0], "\xff\xfe", 2) == 0)
            codepage = 1200;  // UTF-16LE
        else if (fileSize >= 2 && std::memcmp(&ibuf[0], "\xfe\xff", 2) == 0)
            codepage = 1201;  // UTF-16BE

        DWORD ctx = 0;
        UINT size = ibuf.size(), cnt;
        HR(mlang->ConvertStringToUnicode(&ctx, codepage,
                                         &ibuf[0], &size, 0, &cnt));
        std::vector<wchar_t> obuf(cnt);
        size = ibuf.size();
        HR(mlang->ConvertStringToUnicode(&ctx, codepage,
                                         &ibuf[0], &size, &obuf[0], &cnt));
        obuf.push_back(0);
        // chop off BOM
        size_t bom = obuf.size() && obuf[0] == 0xfeff;
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
