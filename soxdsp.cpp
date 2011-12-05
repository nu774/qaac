#include <cstdio>
#include <cstring>
#include <vector>
#include "win32util.h"
#include "soxdsp.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("ERROR"); } \
    while (0)

SoxModule::SoxModule(const std::wstring &path)
{
    HMODULE hDll;
    hDll = LoadLibraryW(path.c_str());
    m_loaded = (hDll != NULL);
    if (!m_loaded)
	return;
    try {
	CHECK(version_string = ProcAddress(hDll, "lsx_rate_version_string"));
	CHECK(rate_create = ProcAddress(hDll, "lsx_rate_create"));
	CHECK(rate_close = ProcAddress(hDll, "lsx_rate_close"));
	CHECK(rate_config = ProcAddress(hDll, "lsx_rate_config"));
	CHECK(rate_start = ProcAddress(hDll, "lsx_rate_start"));
	CHECK(rate_process =
	      ProcAddress(hDll, "lsx_rate_process_noninterleaved"));
	CHECK(fir_create = ProcAddress(hDll, "lsx_fir_create"));
	CHECK(fir_close = ProcAddress(hDll, "lsx_fir_close"));
	CHECK(fir_start = ProcAddress(hDll, "lsx_fir_start"));
	CHECK(fir_process =
	      ProcAddress(hDll, "lsx_fir_process_noninterleaved")); 
	CHECK(design_lpf = ProcAddress(hDll, "lsx_design_lpf"));
	CHECK(free = ProcAddress(hDll, "lsx_free"));
    } catch (...) {
	FreeLibrary(hDll);
	m_loaded = false;
	return;
    }
    m_module = module_t(hDll, FreeLibrary);
}

SoxDSPProcessor::SoxDSPProcessor(const x::shared_ptr<ISoxDSPEngine> &engine,
				 const x::shared_ptr<ISource> &src)
    : DelegatingSource(src), m_engine(engine),
      m_end_of_input(false), m_input_frames(0)
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
    if (srcFormat.m_bitsPerSample == 64)
	throw std::runtime_error("Can't handle 64bit sample");

    m_format = m_engine->getSampleFormat();

    m_fbuffer.resize(4096 * m_format.m_nchannels);
    m_ibuffer.resize(m_fbuffer.size() * m_format.bytesPerFrame());
}

size_t SoxDSPProcessor::readSamples(void *buffer, size_t nsamples)
{
    float *src = &m_fbuffer[0];
    float *dst = static_cast<float*>(buffer);

    unsigned nchannels = m_format.m_nchannels;
    std::vector<float*> ivec(nchannels), ovec(nchannels);

    while (nsamples > 0) {
	if (m_input_frames == 0 && !m_end_of_input) {
	    m_input_frames =
		readSamplesAsFloat(source(), &m_ibuffer, &m_fbuffer, nsamples);
	    if (!m_input_frames)
		m_end_of_input = true;
	    src = &m_fbuffer[0];
	}
	size_t ilen = m_input_frames;
	size_t olen = nsamples;
	for (size_t i = 0; i < nchannels; ++i) {
	    ivec[i] = src + i;
	    ovec[i] = dst + i;
	}
	m_engine->process(&ivec[0], &ovec[0],
			  &ilen, &olen, nchannels, nchannels);
	nsamples -= olen;
	m_input_frames -= ilen;
	src += ilen * nchannels;
	if (m_end_of_input && olen == 0)
	    break;
	dst += olen * nchannels;
    }
    if (m_input_frames) {
	std::memmove(&this->m_fbuffer[0], src,
		     m_input_frames * nchannels * sizeof(float));
    }
    return (dst - static_cast<float*>(buffer)) / nchannels;
}

SoxResampler::SoxResampler(const SoxModule &module, const SampleFormat &format,
			   uint32_t rate, bool mt)
    : m_module(module)
{
    m_format = SampleFormat("F32LE", format.m_nchannels, rate);
    lsx_rate_t *converter = m_module.rate_create(
	    format.m_nchannels, format.m_rate, rate);
    if (!converter)
	throw std::runtime_error("ERROR: SoxResampler");
    m_processor = x::shared_ptr<lsx_rate_t>(converter, m_module.rate_close);
    m_module.rate_config(converter, SOX_RATE_USE_THREADS, static_cast<int>(mt));
    if (m_module.rate_start(converter) < 0)
	throw std::runtime_error("ERROR: SoxResampler");
}

SoxLowpassFilter::SoxLowpassFilter(const SoxModule &module,
			   const SampleFormat &format, uint32_t Fp, bool mt)
    : m_module(module)
{
    m_format = SampleFormat("F32LE", format.m_nchannels, format.m_rate);
    double Fn = static_cast<double>(format.m_rate) / 2.0;
    double Fc = Fp + format.m_rate * 0.005;
    if (Fp == 0 || Fc > Fn)
	throw std::runtime_error("SoxLowpassFilter: invalid target rate");
    int num_taps = 0;
    double *coefs = m_module.design_lpf(Fp, Fc, Fn, 0, 120.0, &num_taps, 0);
    if (!coefs)
	throw std::runtime_error("SoxLowpassFilter: failed to design lpf");
    x::shared_ptr<double> __delete_lator__(coefs, m_module.free);
    lsx_fir_t *converter =
	m_module.fir_create(format.m_nchannels, coefs, num_taps,
			    num_taps >> 1, mt);
    if (!converter)
	throw std::runtime_error("ERROR: SoxLowpassFilter");
    m_processor = x::shared_ptr<lsx_fir_t>(converter, m_module.fir_close);
    if (m_module.fir_start(converter) < 0)
	throw std::runtime_error("ERROR: SoxLowpassFilter");
}
