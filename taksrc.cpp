#include <io.h>
#include <sys/stat.h>
#include "taksrc.h"
#include "strutil.h"
#include "win32util.h"
#include "metadata.h"
#include "cuesheet.h"
#include <apefile.h>
#include <apetag.h>
#include "taglibhelper.h"
#include "cautil.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)

TakModule::TakModule(const std::wstring &path)
    : m_dl(path), m_compatible(false)
{
    if (!m_dl.loaded())
        return;
    try {
        CHECK(GetLibraryVersion = m_dl.fetch("tak_GetLibraryVersion"));
        CHECK(SSD_Create_FromStream = m_dl.fetch("tak_SSD_Create_FromStream"));
        CHECK(SSD_Destroy = m_dl.fetch("tak_SSD_Destroy"));
        CHECK(SSD_GetStreamInfo = m_dl.fetch("tak_SSD_GetStreamInfo"));
        SSD_GetStreamInfo_V22 = m_dl.fetch("tak_SSD_GetStreamInfo_V22");
        CHECK(SSD_Seek = m_dl.fetch("tak_SSD_Seek"));
        CHECK(SSD_ReadAudio = m_dl.fetch("tak_SSD_ReadAudio"));
        CHECK(SSD_GetReadPos = m_dl.fetch("tak_SSD_GetReadPos"));
        TtakInt32 ver, comp;
        GetLibraryVersion(&ver, &comp);
        m_compatible = (comp <= tak_InterfaceVersion
                        && tak_InterfaceVersion <= ver);
    } catch (...) {
        m_dl.reset();
    }
}

namespace tak {
    template <typename T> void try__(T expr, const char *s)
    {
        if (expr != tak_res_Ok) throw std::runtime_error(s);
    }
}

#define TRYTAK(expr) (void)(tak::try__((expr), #expr))

struct TakStreamIoInterfaceImpl: public TtakStreamIoInterface {
    TakStreamIoInterfaceImpl() {
        static TtakStreamIoInterface t = {
            readable, writable, seekable, read,
            0/*write*/, 0/*flush*/, 0/*truncate*/, seek, size
        };
        std::memcpy(this, &t, sizeof(TtakStreamIoInterface));
    }
    static TtakBool readable(void *cookie)
    {
        return tak_True;
    }
    static TtakBool writable(void *cookie)
    {
        return tak_False;
    }
    static TtakBool seekable(void *cookie)
    {
        return util::is_seekable(reinterpret_cast<int>(cookie));
    }
    static TtakBool read(void *cookie, void *buf, TtakInt32 n, TtakInt32 *nr)
    {
        int fd = reinterpret_cast<int>(cookie);
        *nr = util::nread(fd, buf, n);
        return *nr >= 0;
    }
    static TtakBool seek(void *cookie, TtakInt64 pos)
    {
        int fd = reinterpret_cast<int>(cookie);
        return _lseeki64(fd, pos, SEEK_SET) == pos;
    }
    static TtakBool size(void *cookie, TtakInt64 *len)
    {
        int fd = reinterpret_cast<int>(cookie);
        *len = _filelengthi64(fd);
        return *len >= 0LL;
    }
};

TakSource::TakSource(const TakModule &module, const std::shared_ptr<FILE> &fp)
    : m_fp(fp), m_module(module)
{
    static TakStreamIoInterfaceImpl io;
    TtakSSDOptions options = { tak_Cpu_Any, 0 };
    void *ctx = reinterpret_cast<void*>(fileno(m_fp.get()));
    TtakSeekableStreamDecoder ssd =
        m_module.SSD_Create_FromStream(&io, ctx, &options,
                                       staticDamageCallback, this);
    if (!ssd)
        throw std::runtime_error("tak_SSD_Create_FromStream");
    m_decoder = std::shared_ptr<void>(ssd, m_module.SSD_Destroy);

    Ttak_str_StreamInfo_V22 info = { 0 };
    if (m_module.SSD_GetStreamInfo_V22)
        TRYTAK(m_module.SSD_GetStreamInfo_V22(ssd, &info));
    else {
        Ttak_str_StreamInfo_V10 *p =
            reinterpret_cast<Ttak_str_StreamInfo_V10*>(&info);
        TRYTAK(m_module.SSD_GetStreamInfo(ssd, p));
    }
    uint32_t type =
        info.Audio.SampleBits == 8 ? 0 : kAudioFormatFlagIsSignedInteger;
    uint32_t bits = info.Audio.HasExtension ? info.Audio.ValidBitsPerSample
                                            : info.Audio.SampleBits;
    m_asbd = cautil::buildASBDForPCM2(info.Audio.SampleRate,
                                      info.Audio.ChannelNum,
                                      bits, 32,
                                      kAudioFormatFlagIsSignedInteger);
    m_block_align = info.Audio.BlockSize;
    m_length = info.Sizes.SampleNum;
    if (info.Audio.HasExtension && info.Audio.HasSpeakerAssignment) {
        char *a = info.Audio.SpeakerAssignment;
        for (unsigned i = 0; i < 16 && a[i]; ++i)
            m_chanmap.push_back(a[i]);
    }
    try {
        fetchTags();
    } catch (...) {}
}

void TakSource::seekTo(int64_t count)
{
    TRYTAK(m_module.SSD_Seek(m_decoder.get(), count));
}

int64_t TakSource::getPosition()
{
    return m_module.SSD_GetReadPos(m_decoder.get());
}

size_t TakSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_buffer.size() < nsamples * m_block_align)
        m_buffer.resize(nsamples * m_block_align);

    int32_t nread;
    TRYTAK(m_module.SSD_ReadAudio(m_decoder.get(), &m_buffer[0],
                                  nsamples, &nread));
    if (nread > 0) {
        size_t size = nread * m_block_align;
        uint8_t *bp = &m_buffer[0];
        /* convert to signed */
        if (m_asbd.mBitsPerChannel <= 8) {
            for (size_t i = 0; i < size; ++i)
                bp[i] ^= 0x80;
        }
        util::unpack(bp, buffer, &size,
                     m_block_align / m_asbd.mChannelsPerFrame,
                     m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame);
    }
    return nread;
}

void TakSource::fetchTags()
{
    int fd = fileno(m_fp.get());
    util::FilePositionSaver _(fd);
    lseek(fd, 0, SEEK_SET);
    TagLibX::FDIOStreamReader stream(fd);
    TagLib::APE::File file(&stream, false);

    std::map<std::string, std::string> tags;
    std::wstring cuesheet;

    TagLib::APE::Tag *tag = file.APETag(false);
    const TagLib::APE::ItemListMap &itemListMap = tag->itemListMap();
    TagLib::APE::ItemListMap::ConstIterator it;
    for (it = itemListMap.begin(); it != itemListMap.end(); ++it) {
        if (it->second.type() != TagLib::APE::Item::Text)
            continue;
        std::string key = it->first.toCString();
        std::wstring value = it->second.toString().toWString();
        if (strutil::slower(key) == "cuesheet")
            cuesheet = value;
        else
            tags[key] = strutil::w2us(value);
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
