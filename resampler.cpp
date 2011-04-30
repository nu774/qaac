#include <cmath>
#include "resampler.h"
#include "win32util.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("ERROR"); } \
    while (0)

SpeexResamplerModule::SpeexResamplerModule(const std::wstring &path)
{
    HMODULE hDll;
    hDll = LoadLibraryW(path.c_str());
    m_loaded = (hDll != NULL);
    if (!m_loaded)
	return;
    try {
	CHECK(init = ProcAddress(hDll, "speex_resampler_init"));
	CHECK(destroy = ProcAddress(hDll, "speex_resampler_destroy"));
	CHECK(process_interleaved_float = ProcAddress(hDll,
		    "speex_resampler_process_interleaved_float"));
	CHECK(strerror = ProcAddress(hDll, "speex_resampler_strerror"));
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

SpeexResampler::SpeexResampler(const SpeexResamplerModule &module,
	ISource *src, uint32_t rate, int quality)
    : m_module(module), m_src(src), m_length(0), m_peak(0.0), m_input_frames(0)
{
    const SampleFormat &srcFormat = src->getSampleFormat();
    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
	throw std::runtime_error("Can't handle big endian sample");
    if (srcFormat.m_bitsPerSample == 64)
	throw std::runtime_error("Can't handle 64bit sample");

    m_format = SampleFormat("F32LE", srcFormat.m_nchannels, rate);

    int error;
    SpeexResamplerState *converter = m_module.init(
	    m_format.m_nchannels, srcFormat.m_rate, rate, quality, &error);
    if (!converter)
	throw std::runtime_error(
	    format("SpeexResampler: %s", m_module.strerror(error)));
    m_converter = boost::shared_ptr<SpeexResamplerState>(converter,
		m_module.destroy);

    m_src_buffer.resize(4096);
    m_ibuffer.resize(m_src_buffer.size() * srcFormat.bytesPerFrame());

    wchar_t *tmpname = _wtempnam(GetTempPathX().c_str(), L"qaac.tmp");
    FILE *tmpfile = wfopenx(tmpname, L"wb+");
    TempFileCloser closer(tmpname, tmpfile);
    std::free(tmpname);
    if (!tmpfile)
	throw std::runtime_error(format("tmpfile: %s", std::strerror(errno)));
    m_tmpfile = boost::shared_ptr<FILE>(tmpfile, closer);
}

size_t SpeexResampler::convertSamples(size_t nsamples)
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

size_t SpeexResampler::readSamples(void *buffer, size_t nsamples)
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

size_t SpeexResampler::doConvertSamples(float *buffer, size_t nsamples)
{
    float *src = &m_src_buffer[0];
    float *dst = buffer;

    while (nsamples > 0) {
	if (m_input_frames == 0) {
	    underflow();
	    src = &m_src_buffer[0];
	}
	if (m_input_frames == 0)
	    break;
	uint32_t ilen = m_input_frames;
	uint32_t olen = nsamples;
	m_module.process_interleaved_float(m_converter.get(), src,
		&ilen, dst, &olen);
	nsamples -= olen;
	m_input_frames -= ilen;
	src += ilen * m_format.m_nchannels;
	if (olen == 0)
	    break;
	double peak = m_peak;
	for (float *fp = dst; fp != dst + olen * m_format.m_nchannels; ++fp)
	{
	    double abs = std::fabs(*fp);
	    if (abs > peak) peak = abs;
	}
	m_peak = peak;
	dst += olen * m_format.m_nchannels;
    }
    if (m_input_frames) {
	std::memmove(&this->m_src_buffer[0], src,
	    m_input_frames * m_format.m_nchannels * sizeof(float));
    }
    return (dst - buffer) / m_format.m_nchannels;
}

void SpeexResampler::underflow()
{
    const SampleFormat &srcFormat = m_src->getSampleFormat();
    size_t nsamples = m_src_buffer.size() / m_format.bytesPerFrame();

    if (srcFormat.m_type == SampleFormat::kIsFloat &&
	    srcFormat.m_bitsPerSample == 32) {
	m_input_frames = m_src->readSamples(&m_src_buffer[0], nsamples);
	return;
    }
    m_input_frames = m_src->readSamples(&m_ibuffer[0], nsamples);
    size_t blen = m_input_frames * srcFormat.bytesPerFrame();
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