#include "InputFactory.h"
#include "win32util.h"
#ifdef QAAC
#include "ExtAFSource.h"
#endif
#include "FLACSource.h"
#include "LibSndfileSource.h"
#include "RawSource.h"
#include "TakSource.h"
#include "WaveSource.h"
#include "WavpackSource.h"
#include "MP4Source.h"
#include "AvisynthSource.h"

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
    if (strutil::wslower(ext) == L".avs")
        return std::make_shared<AvisynthSource>(path);

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

    TRY_MAKE_SHARED(MP4Source, fp);
#ifdef QAAC
    TRY_MAKE_SHARED(ExtAFSource, fp);
#endif
    TRY_MAKE_SHARED(FLACSource, fp);
    TRY_MAKE_SHARED(WavpackSource, path);
    TRY_MAKE_SHARED(TakSource, fp);
    TRY_MAKE_SHARED(LibSndfileSource, fp);
    throw std::runtime_error("Not available input file format");
}
