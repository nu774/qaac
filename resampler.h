#ifndef _RESAMPLER_H
#define _RESAMPLER_H

#include <libsoxrate.h>
#include "iointer.h"

struct HINSTANCE__;

class SoxResamplerModule {
    typedef x::shared_ptr<HINSTANCE__> module_t;
    module_t m_module;
    bool m_loaded;
public:
    SoxResamplerModule(): m_loaded(false) {}
    SoxResamplerModule(const std::wstring &path);
    bool loaded() const { return m_loaded; }

    lsx_rate_t * (*create)(unsigned, unsigned, unsigned);
    void (*close)(lsx_rate_t *);
    int (*config)(lsx_rate_t *, lsx_rate_config_e, ...);
    void (*start)(lsx_rate_t *);
    size_t (*process)(lsx_rate_t *, const float *, float *, size_t *, size_t *);
};

class SoxResampler: public DelegatingSource {
    SoxResamplerModule m_module;
    SampleFormat m_format;
    x::shared_ptr<lsx_rate_t> m_converter;
    uint64_t m_length;
    uint64_t m_samples_read;
    double m_peak;
    bool m_end_of_input;
    size_t m_input_frames;
    std::vector<char> m_ibuffer;
    std::vector<float> m_src_buffer;
    x::shared_ptr<FILE> m_tmpfile;
    std::map<uint32_t, std::wstring> m_emptyTags;
public:
    SoxResampler(const SoxResamplerModule &module,
	    const x::shared_ptr<ISource> &src,
	    uint32_t rate, int quality=2);
    uint64_t length() const { return m_length; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    size_t readSamples(void *buffer, size_t nsamples);
    double getPeak() const { return m_peak; }
    size_t convertSamples(size_t nsamples);
    uint64_t samplesRead() { return m_samples_read; }
private:
    size_t doConvertSamples(float *buffer, size_t nsamples);
    bool underflow();
};

#endif
