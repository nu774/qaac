#ifndef _RESAMPLER_H
#define _RESAMPLER_H

#include "shared_ptr.h"
#include <speex/speex_resampler.h>
#include "iointer.h"

struct HINSTANCE__;

class SpeexResamplerModule {
    typedef x::shared_ptr<HINSTANCE__> module_t;
    module_t m_module;
    bool m_loaded;
public:
    SpeexResamplerModule(): m_loaded(false) {}
    SpeexResamplerModule(const std::wstring &path);
    bool loaded() const { return m_loaded; }

    SpeexResamplerState *
	(*init)(spx_uint32_t, spx_uint32_t, spx_uint32_t, int, int *);
    void (*destroy)(SpeexResamplerState *);
    int (*process_interleaved_float)(SpeexResamplerState *,
	    const float *, spx_uint32_t *, float *, spx_uint32_t *);
    int (*skip_zeros)(SpeexResamplerState *);
    int (*reset_mem)(SpeexResamplerState *);
    /* XXX: may be null */
    int (*get_input_latency)(SpeexResamplerState *);
    const char *(*strerror)(int);
};

class SpeexResampler: public DelegatingSource {
    class LatencyDetector {
	double m_irate, m_orate;
	uint64_t m_input_accum, m_output_accum;
    public:
	LatencyDetector():
	    m_irate(0), m_orate(0), m_input_accum(0), m_output_accum(0)
	{}
	void set_sample_rates(uint32_t irate, uint32_t orate) {
	    m_irate = irate;
	    m_orate = orate;
	}
	void update(size_t ilen, size_t olen) {
	    m_input_accum += ilen;
	    m_output_accum += olen;
	}
	size_t guess_input_latency() {
	    return static_cast<size_t>(m_input_accum
		    - (m_output_accum * m_irate / m_orate) + 0.5);
	}
    };
    SpeexResamplerModule m_module;
    SampleFormat m_format;
    x::shared_ptr<SpeexResamplerState> m_converter;
    uint64_t m_length;
    double m_peak;
    bool m_end_of_input;
    size_t m_input_frames;
    std::vector<char> m_ibuffer;
    std::vector<float> m_src_buffer;
    x::shared_ptr<FILE> m_tmpfile;
    std::map<uint32_t, std::wstring> m_emptyTags;
    LatencyDetector m_latency_detector;
public:
    SpeexResampler(const SpeexResamplerModule &module,
	    const x::shared_ptr<ISource> &src,
	    uint32_t rate, int quality=3);
    uint64_t length() const { return m_length; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    size_t readSamples(void *buffer, size_t nsamples);
    double getPeak() const { return m_peak; }
    size_t convertSamples(size_t nsamples);
private:
    size_t doConvertSamples(float *buffer, size_t nsamples);
    bool underflow();
};

#endif
