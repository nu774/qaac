#include <vector>
#include "win32util.h"
#include <shlwapi.h>
#include <mlang.h>
#include "shared_ptr.h"

#ifdef __MINGW32__
const GUID IID_IMultiLanguage2 = { 0xdccfc164, 0x2b38, 0x11d2,
    { 0xb7, 0xec, 0x00, 0xc0, 0x4f,0x8f,0x5d,0x9a } };
#endif

static inline
void throwIfError(HRESULT expr, const char *msg)
{
    if (FAILED(expr))
        throw_win32_error(msg, expr);
}
#define HR(expr) (void)(throwIfError((expr), #expr))

std::wstring load_text_file(const std::wstring &path, uint32_t codepage)
{
    struct F {
        static void release(IUnknown *x) {  x->Release(); }
    };

    IStream *stream;
    HRESULT hr = SHCreateStreamOnFileW(path.c_str(),
                                       STGM_READ | STGM_SHARE_DENY_WRITE,
                                       &stream);
    if (FAILED(hr)) throw_win32_error(path, hr);
    x::shared_ptr<IStream> streamPtr(stream, F::release);

    LARGE_INTEGER li = { 0 };
    ULARGE_INTEGER ui;
    HR(stream->Seek(li, STREAM_SEEK_END, &ui));
    if (ui.QuadPart > 0x100000) {
        throw std::runtime_error(w2m(path + L": file too big",
                                     utf8_codecvt_facet()));
    }
    size_t fileSize = ui.LowPart;
    HR(stream->Seek(li, STREAM_SEEK_SET, &ui));

    IMultiLanguage2 *mlang;
    HR(CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_INPROC_SERVER,
                IID_IMultiLanguage2, (void**)(&mlang)));
    x::shared_ptr<IMultiLanguage2> mlangPtr(mlang, F::release);

    if (!codepage) {
        DetectEncodingInfo encoding[5];
        INT nscores = 5;
        HR(mlang->DetectCodepageInIStream(0, GetACP(),
                                          stream, encoding, &nscores));
        /*
         * Usually DetectCodepageInIStream() puts the most appropriate choice
         * in the first place.
         * However, it tends to pick 8bit locale charset for the first place,
         * even if it is really an UTF-8 encoded file.
         */
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
    return normalize_crlf(&obuf[bom], L"\n");
}
