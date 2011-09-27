#include "wvpacksrc.h"
#include <wavpack.h>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "itunetags.h"
#include "cuesheet.h"
#include "win32util.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("ERROR"); } \
    while (0)

WavpackModule::WavpackModule(const std::wstring &path)
{
    HMODULE hDll;
    hDll = LoadLibraryW(path.c_str());
    m_loaded = (hDll != NULL);
    if (!m_loaded)
	return;
    try {
	CHECK(GetLibraryVersionString =
		ProcAddress(hDll, "WavpackGetLibraryVersionString"));
	CHECK(OpenFileInputEx = ProcAddress(hDll, "WavpackOpenFileInputEx"));
	CHECK(CloseFile = ProcAddress(hDll, "WavpackCloseFile"));
	CHECK(GetMode = ProcAddress(hDll, "WavpackGetMode"));
	CHECK(GetNumChannels = ProcAddress(hDll, "WavpackGetNumChannels"));
	CHECK(GetSampleRate = ProcAddress(hDll, "WavpackGetSampleRate"));
	CHECK(GetBitsPerSample =
		ProcAddress(hDll, "WavpackGetBitsPerSample"));
	CHECK(GetNumSamples = ProcAddress(hDll, "WavpackGetNumSamples"));
	CHECK(GetChannelMask = ProcAddress(hDll, "WavpackGetChannelMask"));
	CHECK(GetNumTagItems = ProcAddress(hDll, "WavpackGetNumTagItems"));
	CHECK(GetTagItem = ProcAddress(hDll, "WavpackGetTagItem"));
	CHECK(GetTagItemIndexed =
		ProcAddress(hDll, "WavpackGetTagItemIndexed"));
	CHECK(SeekSample = ProcAddress(hDll, "WavpackSeekSample"));
	CHECK(UnpackSamples = ProcAddress(hDll, "WavpackUnpackSamples"));
    } catch (...) {
	FreeLibrary(hDll);
	m_loaded = false;
	return;
    }
    m_module = module_t(hDll, FreeLibrary);
}

WavpackSource::WavpackSource(const WavpackModule &module, InputStream &stream)
    : m_module(module), m_stream(stream)
{
    static WavpackStreamReader reader = {
	f_read, f_tell, f_seek_abs, f_seek,
	f_pushback, f_size, f_seekable, f_write
    };
    char error[0x100];
    WavpackContext *wpc = m_module.OpenFileInputEx(&reader,
	    &m_stream, 0, error, OPEN_TAGS | OPEN_NORMALIZE, 0);
    if (!wpc)
	throw std::runtime_error(format("WavpackOpenFileInputEx: %s",
		    error));
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
    for (size_t i = 0; i < 32; ++i, mask >>= 1)
	if (mask & 1) m_chanmap.push_back(i + 1);

    fetchTags();
}

void WavpackSource::skipSamples(int64_t count)
{
    if (!m_module.SeekSample(m_wpc.get(),
		static_cast<int32_t>(getSamplesRead() + count)))
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
