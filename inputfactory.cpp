#include "inputfactory.h"
#include "win32util.h"
#ifdef QAAC
#include "afsource.h"
#endif
#include "flacsrc.h"
#include "libsndfilesrc.h"
#include "rawsource.h"
#include "taksrc.h"
#include "wavsource.h"
#include "wvpacksrc.h"
#include "MP4Source.h"

namespace input {
    std::shared_ptr<ISeekableSource> InputFactory::open(const wchar_t *path)
    {
        std::map<std::wstring, std::shared_ptr<ISeekableSource> >::iterator
            pos = m_sources.find(path);
        if (pos != m_sources.end())
            return pos->second;

        const wchar_t *ext = PathFindExtensionW(path);
        std::shared_ptr<FILE> fp(win32::fopen(path, L"rb"));
        if (m_is_raw) {
            std::shared_ptr<RawSource> src =
                std::make_shared<RawSource>(fp, m_raw_format);
            m_sources[path] = src;
            return src;
        }
        if (avisynth.loaded() && strutil::wslower(ext) == L".avs")
            return std::make_shared<AvisynthSource>(avisynth, path);

#define TRY_MAKE_SHARED(type, ...) \
        do { \
            try { \
                std::shared_ptr<type> src = \
                    std::make_shared<type>(__VA_ARGS__); \
                m_sources[path] = src; \
                return src; \
            } catch (...) { \
                _lseeki64(fileno(fp.get()), 0, SEEK_SET); \
            } \
        } while (0)

        TRY_MAKE_SHARED(WaveSource, fp, m_ignore_length);
        if (!win32::is_seekable(fileno(fp.get())))
            throw std::runtime_error("Not available input file format");
        if (libflac.loaded())
            TRY_MAKE_SHARED(FLACSource, libflac, fp);

        if (libwavpack.loaded())
            TRY_MAKE_SHARED(WavpackSource, libwavpack, path);

        if (libtak.loaded() && libtak.compatible())
            TRY_MAKE_SHARED(TakSource, libtak, fp);

        TRY_MAKE_SHARED(MP4Source, fp);
#ifdef QAAC
        TRY_MAKE_SHARED(ExtAFSource, fp);
#endif
        if (libsndfile.loaded())
            TRY_MAKE_SHARED(LibSndfileSource, libsndfile, fp);

        throw std::runtime_error("Not available input file format");
    }
}
