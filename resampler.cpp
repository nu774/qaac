#include <cstdio>
#include <cstring>
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
	CHECK(version_string = ProcAddress(hDll, "lsx_rate_version_string"));
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
	const x::shared_ptr<ISource> &src, uint32_t rate)
    : DelegatingSource(src), m_module(module),
      m_end_of_input(false), m_input_frames(0)
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
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
}

size_t SoxResampler::readSamples(void *buffer, size_t nsamples)
{
    float *src = &m_src_buffer[0];
    float *dst = static_cast<float*>(buffer);

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
	dst += olen * m_format.m_nchannels;
    }
    if (m_input_frames) {
	std::memmove(&this->m_src_buffer[0], src,
	    m_input_frames * m_format.m_nchannels * sizeof(float));
    }
    return (dst - static_cast<float*>(buffer)) / m_format.m_nchannels;
}

bool SoxResampler::underflow()
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
    size_t nsamples = m_src_buffer.size() / m_format.m_nchannels;

    if (srcFormat.m_type == SampleFormat::kIsFloat) {
	m_input_frames = source()->readSamples(&m_src_buffer[0], nsamples);
	if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
	    bswap32buffer(reinterpret_cast<uint8_t*>(&m_src_buffer[0]),
		    nsamples * srcFormat.bytesPerFrame());
	return m_input_frames > 0;
    }
    m_input_frames = source()->readSamples(&m_ibuffer[0], nsamples);
    size_t blen = m_input_frames * srcFormat.bytesPerFrame();
    float *fp = &m_src_buffer[0];

    switch (srcFormat.m_bitsPerSample) {
    case 8:
	{
	    if (srcFormat.m_type == SampleFormat::kIsUnsignedInteger) {
		uint8_t *src = &m_ibuffer[0];
		for (size_t i = 0; i < blen; ++i)
		    src[i] = src[i] ^ 0x80;
	    }
	    const char *src = reinterpret_cast<const char *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x80;
	}
	break;
    case 16:
	{
	    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
		bswap16buffer(&m_ibuffer[0], blen);
	    const short *src = reinterpret_cast<const short *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen / 2; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x8000;
	}
	break;
    case 24:
	{
	    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
		bswap24buffer(&m_ibuffer[0], blen);
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
	    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
		bswap32buffer(&m_ibuffer[0], blen);
	    const int *src = reinterpret_cast<const int *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen / 4; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x80000000U;
	}
	break;
    }
    return m_input_frames > 0;
}
