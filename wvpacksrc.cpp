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
    m_module.swap(module_t(hDll, FreeLibrary));
}

WavpackSource::WavpackSource(const WavpackModule &module, InputStream &stream)
    : m_module(module), m_stream(stream), m_samples_read(0)
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
    m_wpc.swap(std::tr1::shared_ptr<WavpackContext>(wpc, m_module.CloseFile));

    int mode = m_module.GetMode(wpc);
    if (mode & MODE_FLOAT)
	m_format.m_type = SampleFormat::kIsFloat;
    else
	m_format.m_type = SampleFormat::kIsSignedInteger;

    m_format.m_endian = SampleFormat::kIsLittleEndian;
    m_format.m_nchannels = m_module.GetNumChannels(wpc);
    m_format.m_rate = m_module.GetSampleRate(wpc);
    m_format.m_bitsPerSample = m_module.GetBitsPerSample(wpc);
    m_duration = m_module.GetNumSamples(wpc);
    if (m_duration == 0xffffffff)
	m_duration = -1LL;

    unsigned mask = m_module.GetChannelMask(wpc);
    for (size_t i = 0; i < 32; ++i, mask >>= 1)
	if (mask & 1) m_chanmap.push_back(i + 1);

    fetchTags();
}

void WavpackSource::setRange(int64_t start, int64_t length)
{
    int64_t dur = static_cast<int64_t>(m_duration);
    if (length >= 0 && (dur == -1 || length < dur))
	m_duration = length;
    if (start > 0 &&
	    !m_module.SeekSample(m_wpc.get(), static_cast<int32_t>(start)))
	throw std::runtime_error("WavpackSeekSample failed");
    if (start > 0 && dur > 0 && length == -1)
	m_duration -= start;
}

template <class MemorySink>
size_t WavpackSource::readSamplesT(void *buffer, size_t nsamples)
{
    uint64_t rest = m_duration - m_samples_read;
    nsamples = static_cast<size_t>(
	    std::min(static_cast<uint64_t>(nsamples), rest));
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
    m_samples_read += total;
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
    for (int i = 0; i < count; ++i) {
	int size = m_module.GetTagItemIndexed(wpc, i, 0, 0);
	std::vector<char> name(size + 1);
	m_module.GetTagItemIndexed(wpc, i, &name[0], name.size());
	size = m_module.GetTagItem(wpc, &name[0], 0, 0);
	std::vector<char> value(size + 1);
	m_module.GetTagItem(wpc, &name[0], &value[0], value.size());
	uint32_t id = GetIDFromTagName(&name[0]);
	std::wstring wvalue = m2w(&value[0], u8codec);
	if (id)
	    m_tags[id] = wvalue;
	else if (!strcasecmp(&name[0], "cuesheet")) {
	    try {
		CueSheetToChapters(wvalue, m_format.m_rate,
			m_duration, &m_chapters);
	    } catch (...) {}
	}
    }
}
