#include <cstdio>
#include <cstring>
#include <cmath>
#include "win32util.h"
#include "resampler.h"

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
	CHECK(skip_zeros = ProcAddress(hDll, "speex_resampler_skip_zeros"));
	CHECK(reset_mem = ProcAddress(hDll, "speex_resampler_reset_mem"));
	CHECK(strerror = ProcAddress(hDll, "speex_resampler_strerror"));
	/* XXX: not officialy exported function */
	get_input_latency = ProcAddress(hDll, "speex_resampler_get_input_latency");
    } catch (...) {
	FreeLibrary(hDll);
	m_loaded = false;
	return;
    }
    m_module = module_t(hDll, FreeLibrary);
}

SpeexResampler::SpeexResampler(const SpeexResamplerModule &module,
	const x::shared_ptr<ISource> &src, uint32_t rate, int quality)
    : DelegatingSource(src), m_module(module), m_length(0), m_peak(0.0),
      m_end_of_input(false), m_input_frames(0)
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
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
    m_converter = x::shared_ptr<SpeexResamplerState>(converter,
		m_module.destroy);
    m_module.skip_zeros(converter);

    m_src_buffer.resize(4096 * srcFormat.m_nchannels);
    m_ibuffer.resize(m_src_buffer.size() * srcFormat.bytesPerFrame());

    FILE *tmpfile = win32_tmpfile(L"qaac.tmp");
    m_tmpfile = x::shared_ptr<FILE>(tmpfile, std::fclose);

    m_latency_detector.set_sample_rates(srcFormat.m_rate, rate);
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
	if (m_input_frames == 0 && !m_end_of_input) {
	    if (!underflow()) {
		m_end_of_input = true;
		m_input_frames = m_module.get_input_latency
		    ? m_module.get_input_latency(m_converter.get())
		    : m_latency_detector.guess_input_latency();
		std::fill(m_src_buffer.begin(), m_src_buffer.end(), 0.0f);
	    }
	    src = &m_src_buffer[0];
	}
	if (m_input_frames == 0)
	    break;
	uint32_t ilen = m_input_frames;
	uint32_t olen = nsamples;
	m_module.process_interleaved_float(m_converter.get(), src,
		&ilen, dst, &olen);
	m_latency_detector.update(ilen, olen);
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

bool SpeexResampler::underflow()
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
    size_t nsamples = m_src_buffer.size() / m_format.m_nchannels;

    if (srcFormat.m_type == SampleFormat::kIsFloat &&
	    srcFormat.m_bitsPerSample == 32) {
	m_input_frames = source()->readSamples(&m_src_buffer[0], nsamples);
	return m_input_frames > 0;
    }
    m_input_frames = source()->readSamples(&m_ibuffer[0], nsamples);
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
