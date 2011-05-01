#ifndef _RESAMPLER_H
#define _RESAMPLER_H

#include <boost/shared_ptr.hpp>
#include <speex/speex_resampler.h>
#include "iointer.h"

struct HINSTANCE__;

class SpeexResamplerModule {
    typedef boost::shared_ptr<HINSTANCE__> module_t;
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
    const char *(*strerror)(int);
};

class SpeexResampler: public ISource, public ITagParser {
    SpeexResamplerModule m_module;
    ISource *m_src;
    SampleFormat m_format;
    boost::shared_ptr<SpeexResamplerState> m_converter;
    std::vector<char> m_ibuffer;
    std::vector<float> m_src_buffer;
    size_t m_input_frames;
    boost::shared_ptr<FILE> m_tmpfile;
    uint64_t m_length;
    double m_peak;
    std::map<uint32_t, std::wstring> m_emptyTags;
public:
    SpeexResampler(const SpeexResamplerModule &module, ISource *src,
	    uint32_t rate, int quality=3);
    uint64_t length() const { return m_length; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples);
    double getPeak() const { return m_peak; }
    size_t convertSamples(size_t nsamples);
    const std::map<uint32_t, std::wstring> &getTags() const
    {
	ITagParser *parser = dynamic_cast<ITagParser*>(m_src);
	if (!parser)
	    return m_emptyTags;
	else
	    return parser->getTags();
    }
    const std::vector<std::pair<std::wstring, int64_t> > * getChapters() const
    {
	ITagParser *parser = dynamic_cast<ITagParser*>(m_src);
	if (!parser)
	    return 0;
	else
	    return parser->getChapters();
    }
private:
    size_t doConvertSamples(float *buffer, size_t nsamples);
    void underflow();
};

#endif
