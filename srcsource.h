#ifndef _SRCSOURCE_H
#define _SRCSOURCE_H

#ifdef ENABLE_SRC

#include <boost/shared_ptr.hpp>
#include <samplerate.h>
#include "iointer.h"

struct HINSTANCE__;

class SRCModule {
    typedef boost::shared_ptr<HINSTANCE__> module_t;
    module_t m_module;
    bool m_loaded;
public:
    SRCModule(): m_loaded(false) {}
    SRCModule(const std::wstring &path);
    bool loaded() const { return m_loaded; }

    SRC_STATE *(*src_new)(int, int, int*);
    SRC_STATE *(*src_delete)(SRC_STATE*);
    int (*src_process)(SRC_STATE*, SRC_DATA*);
};

class SRCSource: public ISource, public ITagParser {
    SRCModule m_module;
    ISource *m_src;
    SampleFormat m_format;
    boost::shared_ptr<SRC_STATE_tag> m_converter;
    SRC_DATA m_conversion_data;
    std::vector<char> m_ibuffer;
    std::vector<float> m_src_buffer;
    boost::shared_ptr<FILE> m_tmpfile;
    uint64_t m_length;
    double m_peak;
    std::map<uint32_t, std::wstring> m_emptyTags;
public:
    SRCSource(const SRCModule &module, ISource *src, uint32_t rate,
	    int mode=1);
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
    void underflow(size_t nsamples);
};

#endif

#endif
