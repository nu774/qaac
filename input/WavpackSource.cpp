#include <io.h>
#include <sys/stat.h>
#include "WavpackSource.h"
#include <wavpack.h>
#include "strutil.h"
#include "metadata.h"
#include "chanmap.h"
#include "cautil.h"
#include "win32util.h"
#include "WaveSource.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)

bool WavpackModule::load(const std::wstring &path)
{
    if (!m_dl.load(path)) return false;
    try {
        CHECK(GetLibraryVersionString =
              m_dl.fetch( "WavpackGetLibraryVersionString"));
        CHECK(OpenFileInputEx = m_dl.fetch( "WavpackOpenFileInputEx"));
        OpenFileInputEx64 = m_dl.fetch( "WavpackOpenFileInputEx64");
        CHECK(CloseFile = m_dl.fetch( "WavpackCloseFile"));
        CHECK(GetBitsPerSample = m_dl.fetch( "WavpackGetBitsPerSample"));
        CHECK(GetChannelMask = m_dl.fetch( "WavpackGetChannelMask"));
        CHECK(GetMode = m_dl.fetch( "WavpackGetMode"));
        CHECK(GetNumChannels = m_dl.fetch( "WavpackGetNumChannels"));
        CHECK(GetNumSamples = m_dl.fetch( "WavpackGetNumSamples"));
        GetNumSamples64 = m_dl.fetch( "WavpackGetNumSamples64");
        CHECK(GetNumTagItems = m_dl.fetch( "WavpackGetNumTagItems"));
        CHECK(GetNumBinaryTagItems =
              m_dl.fetch( "WavpackGetNumBinaryTagItems"));
        CHECK(GetSampleIndex = m_dl.fetch( "WavpackGetSampleIndex"));
        GetSampleIndex64 = m_dl.fetch( "WavpackGetSampleIndex64");
        CHECK(GetSampleRate = m_dl.fetch( "WavpackGetSampleRate"));
        CHECK(GetTagItem = m_dl.fetch( "WavpackGetTagItem"));
        CHECK(GetBinaryTagItem = m_dl.fetch( "WavpackGetBinaryTagItem"));
        CHECK(GetTagItemIndexed = m_dl.fetch( "WavpackGetTagItemIndexed"));
        CHECK(GetBinaryTagItemIndexed =
              m_dl.fetch( "WavpackGetBinaryTagItemIndexed"));
        CHECK(GetWrapperLocation = m_dl.fetch( "WavpackGetWrapperLocation"));
        CHECK(SeekSample = m_dl.fetch( "WavpackSeekSample"));
        SeekSample64 = m_dl.fetch( "WavpackSeekSample64");
        CHECK(UnpackSamples = m_dl.fetch( "WavpackUnpackSamples"));
        return true;
    } catch (...) {
        m_dl.reset();
        return false;
    }
}

namespace wavpack {
    static inline int fd(void *cookie)
    {
        return static_cast<int>(reinterpret_cast<intptr_t>(cookie));
    }
    static int32_t read(void *cookie, void *data, int32_t count)
    {
        return util::nread(fd(cookie), data, count);
    }
    static int64_t tell(void *cookie)
    {
        return _lseeki64(fd(cookie), 0, SEEK_CUR);
    }
    static uint32_t tell32(void *cookie)
    {
        int64_t off = tell(cookie);
        return off > 0xffffffffLL ? -1 : off;
    }
    static int seek(void *cookie, int64_t off, int whence)
    {
        return _lseeki64(fd(cookie), off, whence) >= 0 ? 0 : -1;
    }
    static int seek32(void *cookie, int32_t off, int whence)
    {
        return seek(cookie, off, whence);
    }
    static int seek_abs(void *cookie, int64_t pos)
    {
        return seek(cookie, pos, SEEK_SET);
    }
    static int seek_abs32(void *cookie, uint32_t pos)
    {
        return seek_abs(cookie, pos);
    }
    static int pushback(void *cookie, int c)
    {
        seek(cookie, -1, SEEK_CUR); // XXX
        return c;
    }
    static int64_t size(void *cookie)
    {
        return _filelengthi64(fd(cookie));
    }
    static uint32_t size32(void *cookie)
    {
        int64_t sz = size(cookie);
        return sz > 0xffffffffLL ? -1 : sz;
    }
    static int seekable(void *cookie)
    {
        return win32::is_seekable(fd(cookie));
    }
}

WavpackSource::WavpackSource(const std::wstring &path)
    : m_module(WavpackModule::instance())
{
    char error[0x100];
    static WavpackStreamReader reader32 = {
        wavpack::read, wavpack::tell32, wavpack::seek_abs32, wavpack::seek32,
        wavpack::pushback, wavpack::size32, wavpack::seekable, nullptr
    };
    static WavpackStreamReader64 reader64 = {
        wavpack::read, nullptr, wavpack::tell, wavpack::seek_abs,
        wavpack::seek, wavpack::pushback, wavpack::size, wavpack::seekable,
        nullptr, nullptr
    };
    if (!m_module.loaded()) throw std::runtime_error("libwavpack not loaded");
    m_fp = win32::fopen(path, L"rb");
    try { m_cfp = win32::fopen(path + L"c", L"rb"); } catch(...) {}

    int flags = OPEN_TAGS | OPEN_NORMALIZE | OPEN_DSD_AS_PCM
              | (m_cfp.get() ? OPEN_WVC : 0);
    void *ra =
        reinterpret_cast<void*>(static_cast<intptr_t>(_fileno(m_fp.get())));
    void *rc = m_cfp.get() ?
        reinterpret_cast<void*>(static_cast<intptr_t>(_fileno(m_cfp.get())))
        : 0;

    WavpackContext *wpc = nullptr;
    if (m_module.OpenFileInputEx64)
        wpc = m_module.OpenFileInputEx64(&reader64, ra, rc, error, flags, 0);
    else
        wpc = m_module.OpenFileInputEx(&reader32, ra, rc, error, flags, 0);
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

    m_length = m_module.GetNumSamples64 ? m_module.GetNumSamples64(wpc)
                                        : m_module.GetNumSamples(wpc);

    unsigned mask = m_module.GetChannelMask(wpc);
    m_chanmap = chanmap::getChannels(mask, m_asbd.mChannelsPerFrame);

    fetchTags();
}

void WavpackSource::seekTo(int64_t count)
{
    int rc = m_module.SeekSample64 ? m_module.SeekSample64(m_wpc.get(), count)
                                   : m_module.SeekSample(m_wpc.get(), count);
    if (!rc) throw std::runtime_error("WavpackSeekSample()");
}

int64_t WavpackSource::getPosition()
{
    return m_module.GetSampleIndex64 ? m_module.GetSampleIndex64(m_wpc.get())
                                     : m_module.GetSampleIndex(m_wpc.get());
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
    std::map<std::string, std::string> tags;
    for (int i = 0; i < count; ++i) {
        int size = m_module.GetTagItemIndexed(wpc, i, 0, 0);
        std::vector<char> name(size + 1);
        m_module.GetTagItemIndexed(wpc, i, name.data(), name.size());
        size = m_module.GetTagItem(wpc, name.data(), 0, 0);
        std::vector<char> value(size + 1);
        m_module.GetTagItem(wpc, name.data(), value.data(), value.size());
        tags[name.data()] = value.data();
    }
    m_tags = TextBasedTag::normalizeTags(tags);

    count = m_module.GetNumBinaryTagItems(wpc);
    for (int i = 0; i < count; ++i) {
        int size = m_module.GetBinaryTagItemIndexed(wpc, i, 0, 0);
        std::vector<char> name(size + 1);
        m_module.GetBinaryTagItemIndexed(wpc, i, name.data(), name.size());
        if (strcasecmp(name.data(), "Cover Art (Front)"))
            continue;
        size = m_module.GetBinaryTagItem(wpc, name.data(), 0, 0);
        std::vector<char> cover;
        cover.resize(size);
        m_module.GetBinaryTagItem(wpc, name.data(), cover.data(), size);
        // strip filename\0 at the beginning
        auto pos = std::find(cover.begin(), cover.end(), '\0');
        std::rotate(cover.begin(), pos + 1, cover.end());
        cover.resize(cover.end() - pos - 1);
        m_tags["COVER ART"] = std::string(cover.begin(), cover.end());
    }
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
