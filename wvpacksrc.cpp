#include <io.h>
#include <sys/stat.h>
#include "wvpacksrc.h"
#include <wavpack.h>
#include "strutil.h"
#include "metadata.h"
#include "cuesheet.h"
#include "chanmap.h"
#include "cautil.h"
#include "win32util.h"
#include "wavsource.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)

WavpackModule::WavpackModule(const std::wstring &path)
    : m_dl(path)
{
    if (!m_dl.loaded()) return;
    try {
        CHECK(GetLibraryVersionString =
              m_dl.fetch( "WavpackGetLibraryVersionString"));
        CHECK(OpenFileInputEx = m_dl.fetch( "WavpackOpenFileInputEx"));
        CHECK(CloseFile = m_dl.fetch( "WavpackCloseFile"));
        CHECK(GetBitsPerSample = m_dl.fetch( "WavpackGetBitsPerSample"));
        CHECK(GetChannelMask = m_dl.fetch( "WavpackGetChannelMask"));
        CHECK(GetMode = m_dl.fetch( "WavpackGetMode"));
        CHECK(GetNumChannels = m_dl.fetch( "WavpackGetNumChannels"));
        CHECK(GetNumSamples = m_dl.fetch( "WavpackGetNumSamples"));
        CHECK(GetNumTagItems = m_dl.fetch( "WavpackGetNumTagItems"));
        CHECK(GetSampleIndex = m_dl.fetch( "WavpackGetSampleIndex"));
        CHECK(GetSampleRate = m_dl.fetch( "WavpackGetSampleRate"));
        CHECK(GetTagItem = m_dl.fetch( "WavpackGetTagItem"));
        CHECK(GetTagItemIndexed = m_dl.fetch( "WavpackGetTagItemIndexed"));
        CHECK(GetWrapperLocation = m_dl.fetch( "WavpackGetWrapperLocation"));
        CHECK(SeekSample = m_dl.fetch( "WavpackSeekSample"));
        CHECK(UnpackSamples = m_dl.fetch( "WavpackUnpackSamples"));
    } catch (...) {
        m_dl.reset();
    }
}

namespace {
    struct WavpackStreamReaderImpl: public WavpackStreamReader
    {
        WavpackStreamReaderImpl()
        {
            static WavpackStreamReader t = {
                read, tell, seek_abs, seek, pushback, size, seekable, 0/*write*/
            };
            std::memcpy(this, &t, sizeof(WavpackStreamReader));
        }
        static int32_t read(void *cookie, void *data, int32_t count)
        {
            int fd = reinterpret_cast<int>(cookie);
            return util::nread(fd, data, count);
        }
        static uint32_t tell(void *cookie)
        {
            int fd = reinterpret_cast<int>(cookie);
            int64_t off = _lseeki64(fd, 0, SEEK_CUR);
            return std::min(off, 0xffffffffLL); // XXX
        }
        static int seek_abs(void *cookie, uint32_t pos)
        {
            int fd = reinterpret_cast<int>(cookie);
            return _lseeki64(fd, pos, SEEK_SET) >= 0 ? 0 : -1;
        }
        static int seek(void *cookie, int32_t off, int whence)
        {
            int fd = reinterpret_cast<int>(cookie);
            return _lseeki64(fd, off, whence) >= 0 ? 0 : -1;
        }
        static int pushback(void *cookie, int c)
        {
            int fd = reinterpret_cast<int>(cookie);
            _lseeki64(fd, -1, SEEK_CUR); // XXX
            return c;
        }
        static uint32_t size(void *cookie)
        {
            int fd = reinterpret_cast<int>(cookie);
            int64_t size = _filelengthi64(fd);
            return std::min(size, 0xffffffffLL); // XXX
        }
        static int seekable(void *cookie)
        {
            return util::is_seekable(reinterpret_cast<int>(cookie));
        }
    };
}

WavpackSource::WavpackSource(const WavpackModule &module,
                             const std::wstring &path)
    : m_module(module)
{
    char error[0x100];
    /* wavpack doesn't copy WavpackStreamReader into their context, therefore
     * must be kept in the memory */
    static WavpackStreamReaderImpl reader;
    m_fp = win32::fopen(path, L"rb");
    try { m_cfp = win32::fopen(path + L"c", L"rb"); } catch(...) {}

    int flags = OPEN_TAGS | OPEN_NORMALIZE | (m_cfp.get() ? OPEN_WVC : 0);
    void *ra = reinterpret_cast<void*>(fileno(m_fp.get()));
    void *rc =
        m_cfp.get() ? reinterpret_cast<void*>(fileno(m_cfp.get())) : 0;

    WavpackContext *wpc =
        m_module.OpenFileInputEx(&reader, ra, rc, error, flags, 0);
    if (!wpc)
        throw std::runtime_error("WavpackOpenFileInputEx() failed");
    m_wpc.reset(wpc, m_module.CloseFile);

    if (!parseWrapper()) {
        bool is_float = m_module.GetMode(wpc) & MODE_FLOAT;
        uint32_t flags = is_float ? kAudioFormatFlagIsFloat
                                  : kAudioFormatFlagIsSignedInteger;
        uint32_t bits = m_module.GetBitsPerSample(wpc);
        uint32_t obits = (is_float && bits == 16) ? 16 : 32;

        m_asbd = cautil::buildASBDForPCM2(m_module.GetSampleRate(wpc),
                                          m_module.GetNumChannels(wpc),
                                          bits, obits, flags);
    }
    if (m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame == 2)
        m_readSamples = &WavpackSource::readSamples16;
    else
        m_readSamples = &WavpackSource::readSamples32;

    uint64_t duration = m_module.GetNumSamples(wpc);
    if (duration == 0xffffffff) duration = ~0ULL;
    m_length = duration;

    unsigned mask = m_module.GetChannelMask(wpc);
    chanmap::getChannels(mask, &m_chanmap, m_asbd.mChannelsPerFrame);

    fetchTags();
}

void WavpackSource::seekTo(int64_t count)
{
    if (!m_module.SeekSample(m_wpc.get(), static_cast<int32_t>(count)))
        throw std::runtime_error("WavpackSeekSample()");
}

int64_t WavpackSource::getPosition()
{
    return m_module.GetSampleIndex(m_wpc.get());
}

bool WavpackSource::parseWrapper()
{
    int fd = fileno(m_fp.get());
    util::FilePositionSaver saver__(fd);
    _lseeki64(fd, 0, SEEK_SET);

    WavpackHeader hdr;
    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
        return false;
    if (std::memcmp(hdr.ckID, "wvpk", 4) != 0)
        return false;
    if (hdr.ckSize < sizeof(hdr) || hdr.ckSize > 0x1000000)
        return false;
    std::vector<char> first_block(hdr.ckSize);

    _lseeki64(fd, 0, SEEK_SET);
    if (read(fd, &first_block[0], hdr.ckSize) != hdr.ckSize)
        return false;
    void *loc = m_module.GetWrapperLocation(&first_block[0], 0);
    if (!loc)
        return false;
    ptrdiff_t off = static_cast<char *>(loc) - &first_block[0];
    _lseeki64(fd, off, SEEK_SET);

    try {
        WaveSource src(m_fp, false);
        memcpy(&m_asbd, &src.getSampleFormat(), sizeof(m_asbd));
        return true;
    } catch (const std::runtime_error &) {
        return false;
    }
}

void WavpackSource::fetchTags()
{
    WavpackContext *wpc = m_wpc.get();

    int count = m_module.GetNumTagItems(wpc);
    std::wstring cuesheet;
    std::map<std::string, std::string> tags;
    for (int i = 0; i < count; ++i) {
        int size = m_module.GetTagItemIndexed(wpc, i, 0, 0);
        std::vector<char> name(size + 1);
        m_module.GetTagItemIndexed(wpc, i, &name[0], name.size());
        size = m_module.GetTagItem(wpc, &name[0], 0, 0);
        std::vector<char> value(size + 1);
        m_module.GetTagItem(wpc, &name[0], &value[0], value.size());
        if (!strcasecmp(&name[0], "cuesheet"))
            cuesheet = strutil::us2w(&value[0]);
        else
            tags[&name[0]] = &value[0];
    }
    if (cuesheet.size()) {
        std::map<std::string, std::string> ctags;
        Cue::CueSheetToChapters(cuesheet,
                                m_length / m_asbd.mSampleRate,
                                &m_chapters, &ctags);
        std::map<std::string, std::string>::const_iterator it;
        for (it = ctags.begin(); it != ctags.end(); ++it)
            tags[it->first] = it->second;
    }
    TextBasedTag::normalizeTags(tags, &m_tags);
}

size_t WavpackSource::readSamples32(void *buffer, size_t nsamples)
{
    /*
     * Wavpack frame is interleaved and aligned to low at byte level,
     * but aligned to high at bit level inside of valid bytes.
     * 20bits sample is stored like the following:
     * <-- MSB ------------------ LSB ---->
     * 00000000 xxxxxxxx xxxxxxxx xxxx0000
     */
    int shifts = 32 - ((m_asbd.mBitsPerChannel + 7) & ~7);
    int32_t *bp = static_cast<int32_t *>(buffer);
    int rc = m_module.UnpackSamples(m_wpc.get(), bp, nsamples);
    if (rc && shifts) {
        const size_t count = rc * m_asbd.mChannelsPerFrame;
        /* align to MSB side */
        for (size_t i = 0; i < count; ++i)
            bp[i] <<= shifts; 
    }
    return rc;
}

size_t WavpackSource::readSamples16(void *buffer, size_t nsamples)
{
    size_t nbytes = nsamples * m_asbd.mChannelsPerFrame * 4;
    if (m_pivot.size() < nbytes)
        m_pivot.resize(nbytes);
    int32_t *bp = reinterpret_cast<int32_t *>(&m_pivot[0]);
    int rc = m_module.UnpackSamples(m_wpc.get(), bp, nsamples);
    nbytes = rc * m_asbd.mChannelsPerFrame * 4;
    const size_t count = rc * m_asbd.mChannelsPerFrame;
    for (size_t i = 0; i < count; ++i)
        bp[i] <<= 16;
    util::pack(bp, &nbytes, 4, 2);
    memcpy(buffer, bp, nbytes);
    return rc;
}
