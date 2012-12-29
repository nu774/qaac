#include <cstdio>
#include <cstring>
#include <vector>
#include "soxdsp.h"
#include "cautil.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)

SoxModule::SoxModule(const std::wstring &path)
    : m_dl(path)
{
    if (!m_dl.loaded())
        return;
    try {
        CHECK(version_string = m_dl.fetch("lsx_rate_version_string"));
        CHECK(rate_create = m_dl.fetch("lsx_rate_create"));
        CHECK(rate_close = m_dl.fetch("lsx_rate_close"));
        CHECK(rate_config = m_dl.fetch("lsx_rate_config"));
        CHECK(rate_start = m_dl.fetch("lsx_rate_start"));
        CHECK(rate_process = m_dl.fetch("lsx_rate_process_noninterleaved"));
        CHECK(rate_process_d =
              m_dl.fetch("lsx_rate_process_noninterleaved_double"));
        CHECK(fir_create = m_dl.fetch("lsx_fir_create"));
        CHECK(fir_close = m_dl.fetch("lsx_fir_close"));
        CHECK(fir_start = m_dl.fetch("lsx_fir_start"));
        CHECK(fir_process = m_dl.fetch("lsx_fir_process_noninterleaved")); 
        CHECK(fir_process_d =
              m_dl.fetch("lsx_fir_process_noninterleaved_double")); 
        CHECK(design_lpf = m_dl.fetch("lsx_design_lpf"));
        CHECK(free = m_dl.fetch("lsx_free"));
    } catch (...) {
        m_dl.reset();
    }
}

SoxDSPProcessor::SoxDSPProcessor(const std::shared_ptr<ISoxDSPEngine> &engine,
                                 const std::shared_ptr<ISource> &src)
    : FilterBase(src), m_position(0), m_engine(engine)
{
    const AudioStreamBasicDescription &sfmt = source()->getSampleFormat();
    m_asbd = m_engine->getSampleFormat();

    m_buffer.units_per_packet = m_asbd.mChannelsPerFrame;

    double factor = m_asbd.mSampleRate / sfmt.mSampleRate;
    m_length = source()->length();
    if (m_length != ~0ULL)
        m_length = m_length * factor + .5;
}

size_t SoxDSPProcessor::readSamples(void *buffer, size_t nsamples)
{
    unsigned nchannels = m_asbd.mChannelsPerFrame;
    m_buffer.resize(4096);

    double **ivec, **ovec;
    ivec = static_cast<double**>(_alloca(sizeof(double*) * nchannels));
    ovec = static_cast<double**>(_alloca(sizeof(double*) * nchannels));
    double *dst = static_cast<double*>(buffer);
    for (size_t i = 0; i < nchannels; ++i)
        ovec[i] = dst + i;

    size_t ilen = 0, olen = 0;
    /*
     * We return 0 as EOF indicator, so necessary to loop while 
     * DSP is consuming input but not producing output
     */
    do {
        if (m_buffer.count() == 0) {
            size_t n = readSamplesAsFloat(source(), &m_ibuffer,
                                          m_buffer.write_ptr(), 4096);
            m_buffer.commit(n);
        }
        double *src = m_buffer.read_ptr();
        for (size_t i = 0; i < nchannels; ++i)
            ivec[i] = src + i;

        ilen = m_buffer.count();
        olen = nsamples;
        m_engine->process(ivec, ovec, &ilen, &olen, nchannels, nchannels);
        m_buffer.advance(ilen);
    } while (ilen != 0 && olen == 0);

    m_position += olen;
    return olen;
}

SoxResampler::SoxResampler(const SoxModule &module,
                           const AudioStreamBasicDescription &asbd,
                           uint32_t rate, bool mt)
    : m_module(module)
{
    m_factor = rate / asbd.mSampleRate;
    m_asbd = cautil::buildASBDForPCM(rate, asbd.mChannelsPerFrame, 64,
                                     kAudioFormatFlagIsFloat);
    lsx_rate_t *converter =
        m_module.rate_create(asbd.mChannelsPerFrame, asbd.mSampleRate, rate);
    if (!converter)
        throw std::runtime_error("lsx_rate_create()");
    m_processor = std::shared_ptr<lsx_rate_t>(converter, m_module.rate_close);
    m_module.rate_config(converter, SOX_RATE_USE_THREADS, static_cast<int>(mt));
    if (m_module.rate_start(converter) < 0)
        throw std::runtime_error("lsx_rate_config()");
}

SoxLowpassFilter::SoxLowpassFilter(const SoxModule &module,
                                   const AudioStreamBasicDescription &asbd,
                                   uint32_t Fp, bool mt)
    : m_module(module)
{
    m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate, asbd.mChannelsPerFrame,
                                     64, kAudioFormatFlagIsFloat);
    double Fn = asbd.mSampleRate / 2.0;
    double Fs = Fp + asbd.mSampleRate * 0.0125;
    if (Fp == 0 || Fs > Fn)
        throw std::runtime_error("SoxLowpassFilter: invalid target rate");
    int num_taps = 0;
    double *coefs = m_module.design_lpf(Fp, Fs, Fn, 120.0, &num_taps, 0, -1);
    if (!coefs)
        throw std::runtime_error("lsx_design_lpf()");
    std::shared_ptr<double> __delete_lator__(coefs, m_module.free);
    lsx_fir_t *converter =
        m_module.fir_create(asbd.mChannelsPerFrame, coefs, num_taps,
                            num_taps >> 1, mt);
    if (!converter)
        throw std::runtime_error("lsx_fir_create()");
    m_processor = std::shared_ptr<lsx_fir_t>(converter, m_module.fir_close);
    if (m_module.fir_start(converter) < 0)
        throw std::runtime_error("lsx_fir_start()");
}
