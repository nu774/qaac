#ifdef ENABLE_SRC
#include <cmath>
#include "srcsource.h"
#include "win32util.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("ERROR"); } \
    while (0)

SRCModule::SRCModule(const std::wstring &path)
{
    HMODULE hDll;
    hDll = LoadLibraryW(path.c_str());
    m_loaded = (hDll != NULL);
    if (!m_loaded)
	return;
    try {
	CHECK(src_new = ProcAddress(hDll, "src_new"));
	CHECK(src_delete = ProcAddress(hDll, "src_delete"));
	CHECK(src_process = ProcAddress(hDll, "src_process"));
    } catch (...) {
	FreeLibrary(hDll);
	m_loaded = false;
	return;
    }
    m_module = module_t(hDll, FreeLibrary);
}

struct TempFileCloser {
    TempFileCloser(const std::wstring &name, FILE *fp):
    	m_name(name), m_fp(fp)
    {}
    void operator()(void *) {
	std::fclose(m_fp);
	DeleteFileX(m_name.c_str());
    }
    std::wstring m_name;
    FILE *m_fp;
};

SRCSource::SRCSource(const SRCModule &module, ISource *src, uint32_t rate,
	int mode)
    : m_module(module), m_src(src), m_length(0), m_peak(0.0)
{
    const SampleFormat &srcFormat = src->getSampleFormat();
    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
	throw std::runtime_error("Can't handle big endian sample");
    if (srcFormat.m_bitsPerSample == 64)
	throw std::runtime_error("Can't handle 64bit sample");

    m_format = SampleFormat("F32LE", srcFormat.m_nchannels, rate);
    memset(&m_conversion_data, 0, sizeof m_conversion_data);

    int error;
    SRC_STATE *converter = m_module.src_new(mode, m_format.m_nchannels, &error);
    if (!converter) throw std::runtime_error("src_new");
    m_converter = boost::shared_ptr<SRC_STATE_tag>(converter,
		m_module.src_delete);

    m_src_buffer.resize(4096);
    m_ibuffer.resize(m_src_buffer.size() * srcFormat.bytesPerFrame());

    m_conversion_data.src_ratio = 1.0 * rate / srcFormat.m_rate;

    wchar_t *tmpname = _wtempnam(GetTempPathX().c_str(), L"qaac.tmp");
    FILE *tmpfile = wfopenx(tmpname, L"wb+");
    TempFileCloser closer(tmpname, tmpfile);
    std::free(tmpname);
    if (!tmpfile)
	throw std::runtime_error(format("tmpfile: %s", std::strerror(errno)));
    m_tmpfile = boost::shared_ptr<FILE>(tmpfile, closer);
}

size_t SRCSource::convertSamples(size_t nsamples)
{
    std::vector<float> buff(nsamples* m_format.m_nchannels);
    size_t nc;
    if ((nc = doConvertSamples(&buff[0], nsamples)) > 0) {
	std::fwrite(&buff[0], sizeof(float), nc * m_format.m_nchannels,
		m_tmpfile.get());
	m_length += nc;
    }
    else
	std::fseek(m_tmpfile.get(), 0, SEEK_SET);
    return nc;
}

size_t SRCSource::readSamples(void *buffer, size_t nsamples)
{
    size_t nc = std::fread(buffer, sizeof(float),
 	    nsamples * m_format.m_nchannels, m_tmpfile.get());
    float *fp = reinterpret_cast<float*>(buffer);
    if (m_peak > 1.0) {
	for (size_t i = 0; i < nc; ++i) {
	    float nfp = static_cast<float>(*fp / m_peak);
	    *fp++ = nfp;
	}
    }
    return nc / m_format.m_nchannels;
}

size_t SRCSource::doConvertSamples(float *buffer, size_t nsamples)
{
    SRC_DATA &sd = m_conversion_data;
    sd.data_out = buffer;
    sd.output_frames = nsamples;

    while (sd.output_frames > 0) {
	if (sd.input_frames == 0) {
	    sd.data_in = &m_src_buffer[0];
	    underflow(nsamples);
	}
	if (sd.input_frames == 0) {
	    sd.end_of_input = 1;
	}

	int error;
	if ((error = m_module.src_process(m_converter.get(), &sd))) {
	    throw std::runtime_error(format("src_process: %d", error));
	}

	sd.data_in += sd.input_frames_used * m_format.m_nchannels;
	sd.input_frames -= sd.input_frames_used;
	if (sd.output_frames_gen == 0)
	    break;
	double peak = m_peak;
	for (float *fp = sd.data_out;
		fp != sd.data_out + sd.output_frames_gen; ++fp)
	{
	    double abs = std::fabs(*fp);
	    if (abs > peak) peak = abs;
	}
	m_peak = peak;
	sd.data_out += sd.output_frames_gen * m_format.m_nchannels;
	sd.output_frames -= sd.output_frames_gen;
    }
    return (sd.data_out - buffer) / m_format.m_nchannels;
}

void SRCSource::underflow(size_t nsamples)
{
    const SampleFormat &srcFormat = m_src->getSampleFormat();
    nsamples = std::min(nsamples, m_src_buffer.size() / m_format.m_nchannels);

    if (srcFormat.m_type == SampleFormat::kIsFloat &&
	    srcFormat.m_bitsPerSample == 32) {
	m_conversion_data.input_frames =
	    m_src->readSamples(&m_src_buffer[0], nsamples);
	return;
    }
    m_conversion_data.input_frames =
	m_src->readSamples(&m_ibuffer[0], nsamples);
    size_t blen = m_conversion_data.input_frames * srcFormat.bytesPerFrame();
    float *fp = &m_src_buffer[0];

    switch (srcFormat.m_bitsPerSample) {
    case 8:
	{
	    const char *src = reinterpret_cast<const char *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x80;
	}
	break;
    case 16:
	{
	    const short *src = reinterpret_cast<const short *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen / 2; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x8000;
	}
	break;
    case 24:
	{
	    const uint8_t *src =
		reinterpret_cast<const uint8_t*>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen / 3; ++i) {
		int32_t hv = static_cast<int8_t>(src[i*3+2]);
		int32_t v = ((src[i*3] << 8)| (src[i*3+1] << 16) | (hv << 24));
		*fp++ = static_cast<float>(v) / 0x80000000U;
	    }
	}
	break;
    case 32:
	{
	    const int *src = reinterpret_cast<const int *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen / 4; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x80000000U;
	}
	break;
    }
}
#endif
