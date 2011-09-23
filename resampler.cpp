#include <cstdio>
#include <cstring>
#include <cmath>
#include "win32util.h"
#include "resampler.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("ERROR"); } \
    while (0)

SoxResamplerModule::SoxResamplerModule(const std::wstring &path)
{
    HMODULE hDll;
    hDll = LoadLibraryW(path.c_str());
    m_loaded = (hDll != NULL);
    if (!m_loaded)
	return;
    try {
	CHECK(create = ProcAddress(hDll, "lsx_rate_create"));
	CHECK(close = ProcAddress(hDll, "lsx_rate_close"));
	CHECK(config = ProcAddress(hDll, "lsx_rate_config"));
	CHECK(start = ProcAddress(hDll, "lsx_rate_start"));
	CHECK(process = ProcAddress(hDll, "lsx_rate_process"));
    } catch (...) {
	FreeLibrary(hDll);
	m_loaded = false;
	return;
    }
    m_module = module_t(hDll, FreeLibrary);
}

SoxResampler::SoxResampler(const SoxResamplerModule &module,
	const x::shared_ptr<ISource> &src, uint32_t rate, bool normalize)
    : DelegatingSource(src), m_module(module), m_normalize(normalize),
      m_length(0), m_samples_read(0), m_peak(0.0),
      m_end_of_input(false), m_input_frames(0)
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
	throw std::runtime_error("Can't handle big endian sample");
    if (srcFormat.m_bitsPerSample == 64)
	throw std::runtime_error("Can't handle 64bit sample");

    m_format = SampleFormat("F32LE", srcFormat.m_nchannels, rate);

    lsx_rate_t *converter = m_module.create(
	    m_format.m_nchannels, srcFormat.m_rate, rate);
    if (!converter)
	throw std::runtime_error("ERROR: SoxResampler");
    m_converter = x::shared_ptr<lsx_rate_t>(converter, m_module.close);
    if (m_module.start(converter) < 0)
	throw std::runtime_error("ERROR: SoxResampler");

    m_src_buffer.resize(4096 * srcFormat.m_nchannels);
    m_ibuffer.resize(m_src_buffer.size() * srcFormat.bytesPerFrame());

    if (normalize) {
	FILE *tmpfile = win32_tmpfile(L"qaac.SRC");
	m_tmpfile = x::shared_ptr<FILE>(tmpfile, std::fclose);
    } else
	m_length = -1;
}

size_t SoxResampler::convertSamples(size_t nsamples)
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

size_t SoxResampler::readSamples(void *buffer, size_t nsamples)
{
    if (!m_normalize)
	return doConvertSamples(reinterpret_cast<float*>(buffer), nsamples);
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

size_t SoxResampler::doConvertSamples(float *buffer, size_t nsamples)
{
    float *src = &m_src_buffer[0];
    float *dst = buffer;

    while (nsamples > 0) {
	if (m_input_frames == 0 && !m_end_of_input) {
	    if (!underflow())
		m_end_of_input = true;
	    src = &m_src_buffer[0];
	}
	uint32_t ilen = m_input_frames;
	uint32_t olen = nsamples;
	m_module.process(m_converter.get(), src, dst, &ilen, &olen);
	nsamples -= olen;
	m_input_frames -= ilen;
	src += ilen * m_format.m_nchannels;
	if (m_end_of_input && olen == 0)
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

bool SoxResampler::underflow()
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
    size_t nsamples = m_src_buffer.size() / m_format.m_nchannels;

    if (srcFormat.m_type == SampleFormat::kIsFloat &&
	    srcFormat.m_bitsPerSample == 32) {
	m_input_frames = source()->readSamples(&m_src_buffer[0], nsamples);
	m_samples_read += m_input_frames;
	return m_input_frames > 0;
    }
    m_input_frames = source()->readSamples(&m_ibuffer[0], nsamples);
    m_samples_read += m_input_frames;
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
    return m_input_frames > 0;
}
