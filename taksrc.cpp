#include "taksrc.h"
#include "strutil.h"
#include "win32util.h"
#include "itunetags.h"
#include "cuesheet.h"
#include <apefile.h>
#include <apetag.h>
#include "cautil.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)

TakModule::TakModule(const std::wstring &path)
    : m_dl(path)
{
    if (!m_dl.loaded())
	return;
    try {
	CHECK(GetLibraryVersion = m_dl.fetch("tak_GetLibraryVersion"));
	CHECK(SSD_Create_FromStream = m_dl.fetch("tak_SSD_Create_FromStream"));
	CHECK(SSD_Destroy = m_dl.fetch("tak_SSD_Destroy"));
	CHECK(SSD_GetStreamInfo = m_dl.fetch("tak_SSD_GetStreamInfo"));
	CHECK(SSD_Seek = m_dl.fetch("tak_SSD_Seek"));
	CHECK(SSD_ReadAudio = m_dl.fetch("tak_SSD_ReadAudio"));
	CHECK(SSD_GetAPEv2Tag = m_dl.fetch("tak_SSD_GetAPEv2Tag"));
	CHECK(APE_GetItemNum = m_dl.fetch("tak_APE_GetItemNum"));
	CHECK(APE_GetItemKey = m_dl.fetch("tak_APE_GetItemKey"));
	CHECK(APE_GetItemValue = m_dl.fetch("tak_APE_GetItemValue"));
    } catch (...) {
	m_dl.reset();
    }
    TtakInt32 ver, comp;
    GetLibraryVersion(&ver, &comp);
    m_compatible = (comp <= tak_InterfaceVersion
		    && tak_InterfaceVersion <= ver);
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
    static TtakBool readable(void *ctx)
    {
	return tak_True;
    }
    static TtakBool writable(void *ctx)
    {
	return tak_False;
    }
    static TtakBool seekable(void *ctx)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(ctx);
	return pT->seekable();
    }
    static TtakBool read(void *ctx, void *buf, TtakInt32 n, TtakInt32 *nr)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(ctx);
	ssize_t rc = pT->read(buf, n);
	if (nr) *nr = rc;
	return tak_True;
    }
    static TtakBool seek(void *ctx, TtakInt64 pos)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(ctx);
	return pT->seek(pos, ISeekable::kBegin) >= 0;
    }
    static TtakBool size(void *ctx, TtakInt64 *len)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(ctx);
	int64_t rc = pT->size();
	if (len && rc >= 0) *len = rc;
	return rc >= 0;
    }
};

TakSource::TakSource(const TakModule &module, InputStream &stream)
    : m_module(module), m_stream(stream)
{
    static TakStreamIoInterfaceImpl io;
    TtakSSDOptions options = { tak_Cpu_Any, 0 };
    if (!stream.seekable())
	options.Flags |= tak_ssd_opt_SequentialRead;
    TtakSeekableStreamDecoder ssd = m_module.SSD_Create_FromStream(
	    &io, &m_stream, &options, staticDamageCallback, this);
    if (!ssd)
	throw std::runtime_error("tak_SSD_Create_FromStream");
    m_decoder = std::shared_ptr<void>(ssd, m_module.SSD_Destroy);
    Ttak_str_StreamInfo info;
    TRYTAK(m_module.SSD_GetStreamInfo(ssd, &info));
    uint32_t type =
	info.Audio.SampleBits == 8 ? 0 : kAudioFormatFlagIsSignedInteger;
    m_asbd = cautil::buildASBDForPCM(info.Audio.SampleRate,
				info.Audio.ChannelNum,
				info.Audio.SampleBits,
				type,
				kAudioFormatFlagIsAlignedHigh);
    if (m_asbd.mBytesPerFrame != info.Audio.BlockSize)
	throw std::runtime_error(
		strutil::format("blocksize: %d is different from expected",
		    info.Audio.BlockSize));
    setRange(0, info.Sizes.SampleNum);
    if (stream.seekable())
	fetchTags();
}

void TakSource::skipSamples(int64_t count)
{
    TRYTAK(m_module.SSD_Seek(m_decoder.get(),
	PartialSource::getSamplesRead() + count));
}

size_t TakSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    uint8_t *bufp = reinterpret_cast<uint8_t*>(buffer);
    size_t total = 0;
    while (total < nsamples) {
	int32_t nread;
	TRYTAK(m_module.SSD_ReadAudio(
		    m_decoder.get(), bufp, nsamples - total, &nread));
	if (nread == 0)
	    break;
	total += nread;
	bufp += nread * m_asbd.mBytesPerFrame;
    }
    addSamplesRead(total);
    return total;
}

void TakSource::fetchTags()
{
    std::wstring filename = m_stream.name();
#ifdef _WIN32
    std::wstring fullname = win32::prefixed_path(filename.c_str());
#else
    std::string fullname = strutil::w2m(filename);
#endif
    TagLib::APE::File file(fullname.c_str(), false);
    if (!file.isOpen())
	throw std::runtime_error("taglib: can't open file");
    TagLib::APE::Tag *tag = file.APETag(false);
    const TagLib::APE::ItemListMap &itemListMap = tag->itemListMap();
    TagLib::APE::ItemListMap::ConstIterator it;
    for (it = itemListMap.begin(); it != itemListMap.end(); ++it) {
	std::wstring key = it->first.toWString();
	std::string skey = strutil::w2us(key);
	std::wstring value = it->second.toString().toWString();
	uint32_t id = Vorbis::GetIDFromTagName(skey.c_str());
	if (id)
	    m_tags[id] = value;
	else if (!strcasecmp(skey.c_str(), "cuesheet")) {
	    std::map<uint32_t, std::wstring> meta;
	    Cue::CueSheetToChapters(value, m_asbd.mSampleRate,
				    getDuration(), &m_chapters, &meta);
	    std::map<uint32_t, std::wstring>::iterator it;
	    for (it = meta.begin(); it != meta.end(); ++it)
		m_tags[it->first] = it->second;
	}
    }
}
