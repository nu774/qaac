#include "wvpacksrc.h"
#include <wavpack.h>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "itunetags.h"
#include "cuesheet.h"
#include "chanmap.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("ERROR"); } \
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
	CHECK(GetMode = m_dl.fetch( "WavpackGetMode"));
	CHECK(GetNumChannels = m_dl.fetch( "WavpackGetNumChannels"));
	CHECK(GetSampleRate = m_dl.fetch( "WavpackGetSampleRate"));
	CHECK(GetBitsPerSample = m_dl.fetch( "WavpackGetBitsPerSample"));
	CHECK(GetNumSamples = m_dl.fetch( "WavpackGetNumSamples"));
	CHECK(GetChannelMask = m_dl.fetch( "WavpackGetChannelMask"));
	CHECK(GetNumTagItems = m_dl.fetch( "WavpackGetNumTagItems"));
	CHECK(GetTagItem = m_dl.fetch( "WavpackGetTagItem"));
	CHECK(GetTagItemIndexed = m_dl.fetch( "WavpackGetTagItemIndexed"));
	CHECK(SeekSample = m_dl.fetch( "WavpackSeekSample"));
	CHECK(UnpackSamples = m_dl.fetch( "WavpackUnpackSamples"));
    } catch (...) {
	m_dl.reset();
    }
}

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
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->read(data, count);
    }
    static uint32_t tell(void *cookie)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return static_cast<uint32_t>(pT->tell());
    }
    static int seek_abs(void *cookie, uint32_t pos)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->seek(pos, ISeekable::kBegin) >= 0 ? 0 : -1;
    }
    static int seek(void *cookie, int32_t off, int whence)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->seek(off, whence) >= 0 ? 0 : -1;
    }
    static int pushback(void *cookie, int c)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	pT->pushback(c);
	return c;
    }
    static uint32_t size(void *cookie)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->size();
    }
    static int seekable(void *cookie)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->seekable();
    }
};

WavpackSource::WavpackSource(const WavpackModule &module, InputStream &stream,
			     const std::wstring &path)
    : m_module(module), m_stream(stream)
{
    char error[0x100];
    /* wavpack doesn't copy WavpackStreamReader into their context, therefore
     * must be kept in the memory */
    static WavpackStreamReaderImpl reader;

    try {
	std::wstring cpath = path + L"c";
	StdioChannel channel(cpath.c_str());
	m_cstream.reset(new InputStream(channel));
    } catch (...) {}

    int flags = OPEN_TAGS | (m_cstream.get() ? OPEN_WVC : 0);
    WavpackContext *wpc =
	m_module.OpenFileInputEx(&reader, &m_stream, m_cstream.get(),
				 error, flags, 0);
    if (!wpc)
	throw std::runtime_error(format("WavpackOpenFileInputEx: %s", error));
    m_wpc = x::shared_ptr<WavpackContext>(wpc, m_module.CloseFile);

    int mode = m_module.GetMode(wpc);
    if (mode & MODE_FLOAT)
	m_format.m_type = SampleFormat::kIsFloat;
    else
	m_format.m_type = SampleFormat::kIsSignedInteger;

    m_format.m_endian = SampleFormat::kIsLittleEndian;
    m_format.m_nchannels = m_module.GetNumChannels(wpc);
    m_format.m_rate = m_module.GetSampleRate(wpc);
    m_format.m_bitsPerSample = m_module.GetBitsPerSample(wpc);
    uint64_t duration = m_module.GetNumSamples(wpc);
    if (duration == 0xffffffff) duration = -1LL;
    setRange(0, duration);

    unsigned mask = m_module.GetChannelMask(wpc);
    chanmap::GetChannels(mask, &m_chanmap, m_format.m_nchannels);

    fetchTags();
}

void WavpackSource::skipSamples(int64_t count)
{
    if (!m_module.SeekSample(m_wpc.get(),
	static_cast<int32_t>(PartialSource::getSamplesRead() + count)))
	throw std::runtime_error("WavpackSeekSample");

}

template <class MemorySink>
size_t WavpackSource::readSamplesT(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    std::vector<int32_t> vbuf(nsamples * m_format.m_nchannels);
    MemorySink sink(buffer);
    size_t total = 0, rc;
    while (total < nsamples) {
	rc = m_module.UnpackSamples(m_wpc.get(), &vbuf[0], nsamples - total);
	if (rc <= 0)
	    break;
	size_t nblk = rc * m_format.m_nchannels;
	for (size_t i = 0; i < nblk; ++i)
	    sink.put(vbuf[i]);
	total += rc;
    }
    addSamplesRead(total);
    return total;
}

size_t WavpackSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_format.m_bitsPerSample == 8)
	return readSamplesT<MemorySink8>(buffer, nsamples);
    else if (m_format.m_bitsPerSample == 16)
	return readSamplesT<MemorySink16LE>(buffer, nsamples);
    else if (m_format.m_bitsPerSample == 24)
	return readSamplesT<MemorySink24LE>(buffer, nsamples);
    else
	return readSamplesT<MemorySink32LE>(buffer, nsamples);
}

void WavpackSource::fetchTags()
{
    WavpackContext *wpc = m_wpc.get();
    utf8_codecvt_facet u8codec;

    int count = m_module.GetNumTagItems(wpc);
    std::map<std::string, std::string> vorbisComments;
    std::map<uint32_t, std::wstring> tags;
    for (int i = 0; i < count; ++i) {
	int size = m_module.GetTagItemIndexed(wpc, i, 0, 0);
	std::vector<char> name(size + 1);
	m_module.GetTagItemIndexed(wpc, i, &name[0], name.size());
	size = m_module.GetTagItem(wpc, &name[0], 0, 0);
	std::vector<char> value(size + 1);
	m_module.GetTagItem(wpc, &name[0], &value[0], value.size());
	if (!strcasecmp(&name[0], "cuesheet")) {
	    try {
		std::wstring wvalue = m2w(&value[0], u8codec);
		Cue::CueSheetToChapters(wvalue, m_format.m_rate,
			getDuration(), &m_chapters, &tags);
	    } catch (...) {}
	} else {
	    vorbisComments[&name[0]] = &value[0];
	}
    }
    Vorbis::ConvertToItunesTags(vorbisComments, &m_tags);
    std::map<uint32_t, std::wstring>::const_iterator it;
    for (it = tags.begin(); it != tags.end(); ++it)
	m_tags[it->first] = it->second;
}
