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
#include "CAFSource.h"
#include "Win32InputStream.h"

std::shared_ptr<ISeekableSource> InputFactory::open(const wchar_t *path)
{
    std::map<std::wstring, std::shared_ptr<ISeekableSource> >::iterator
        pos = m_sources.find(path);
    if (pos != m_sources.end())
        return pos->second;

    const wchar_t *ext = PathFindExtensionW(path);
    std::shared_ptr<IInputStream> stream = std::make_shared<Win32InputStream>(path);
    if (m_is_raw) {
        std::shared_ptr<RawSource> src =
            std::make_shared<RawSource>(stream, m_raw_format);
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
            stream->seek(0, SEEK_SET); \
        } \
    } while (0)

    TRY_MAKE_SHARED(WaveSource, stream, m_ignore_length);
    TRY_MAKE_SHARED(MP4Source, stream);
    TRY_MAKE_SHARED(CAFSource, stream);
#ifdef QAAC
    TRY_MAKE_SHARED(ExtAFSource, stream);
#endif
    TRY_MAKE_SHARED(FLACSource, stream);
    TRY_MAKE_SHARED(WavpackSource, stream, path);
    TRY_MAKE_SHARED(TakSource, stream);
    TRY_MAKE_SHARED(LibSndfileSource, stream);
    throw std::runtime_error("Not available input file format");
}
