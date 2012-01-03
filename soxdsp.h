#ifndef _RESAMPLER_H
#define _RESAMPLER_H

#include <libsoxrate.h>
#include "iointer.h"
#include "util.h"
#include "dl.h"

class SoxModule {
    DL m_dl;
public:
    SoxModule() {}
    SoxModule(const std::wstring &path);
    bool loaded() const { return m_dl.loaded(); }

    const char *(*version_string)();
    lsx_rate_t *(*rate_create)(unsigned, unsigned, unsigned);
    void (*rate_close)(lsx_rate_t *);
    int (*rate_config)(lsx_rate_t *, lsx_rate_config_e, ...);
    int (*rate_start)(lsx_rate_t *);
    size_t (*rate_process)(lsx_rate_t *, const float * const *, float **,
			   size_t *, size_t *, size_t, size_t);
    lsx_fir_t *(*fir_create)(unsigned, double *, unsigned, unsigned, int);
    int (*fir_close)(lsx_fir_t *);
    int (*fir_start)(lsx_fir_t *);
    int (*fir_process)(lsx_fir_t *, const float * const *, float **,
		       size_t *, size_t *, size_t, size_t);
    double *(*design_lpf)(double, double, double, int, double, int *, int);
    void (*free)(void*);
};

struct ISoxDSPEngine {
    virtual ~ISoxDSPEngine() {}
    virtual const SampleFormat &getSampleFormat() = 0;
    virtual ssize_t process(const float * const *ibuf, float **obuf, size_t *ilen,
			    size_t *olen, size_t istride, size_t ostride) = 0;
};

class SoxDSPProcessor: public DelegatingSource {
    SampleFormat m_format;
    x::shared_ptr<ISoxDSPEngine> m_engine;
    bool m_end_of_input;
    size_t m_input_frames;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float> m_fbuffer;
    uint64_t m_samples_read;
public:
    SoxDSPProcessor(const x::shared_ptr<ISoxDSPEngine> &engine,
		    const x::shared_ptr<ISource> &src);
    uint64_t length() const { return -1; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    size_t readSamples(void *buffer, size_t nsamples);
    uint64_t getSamplesRead() const { return m_samples_read; }
};

class SoxResampler: public ISoxDSPEngine {
    SoxModule m_module;
    x::shared_ptr<lsx_rate_t> m_processor;
    SampleFormat m_format;
public:
    SoxResampler(const SoxModule &module, const SampleFormat &format,
		 uint32_t Fp, bool mt);
    const SampleFormat &getSampleFormat() { return m_format; }
    ssize_t process(const float * const *ibuf, float **obuf, size_t *ilen,
		    size_t *olen, size_t istride, size_t ostride)
    {
	return m_module.rate_process(m_processor.get(), ibuf, obuf,
				     ilen, olen, istride, ostride);
    }
};

class SoxLowpassFilter: public ISoxDSPEngine {
    SoxModule m_module;
    x::shared_ptr<lsx_fir_t> m_processor;
    SampleFormat m_format;
public:
    SoxLowpassFilter(const SoxModule &module, const SampleFormat &format,
		     uint32_t rate, bool mt);
    const SampleFormat &getSampleFormat() { return m_format; }
    ssize_t process(const float * const *ibuf, float **obuf, size_t *ilen,
		    size_t *olen, size_t istride, size_t ostride)
    {
	return m_module.fir_process(m_processor.get(), ibuf, obuf,
				    ilen, olen, istride, ostride);
    }
};
#endif
