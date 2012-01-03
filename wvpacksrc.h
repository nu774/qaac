#ifndef _WVPACKSRC_H
#define _WVPACKSRC_H

#include "ioabst.h"
#include "iointer.h"
#include "dl.h"

/*
 * XXX
 * wavpack.h conflicts with QT header, therefore we don't want to
 * include it here.
 */
typedef void WavpackContext;

class WavpackModule {
    DL m_dl;
public:
    WavpackModule() {}
    explicit WavpackModule(const std::wstring &path);
    bool loaded() const { return m_dl.loaded(); }

    const char *(*GetLibraryVersionString)();
    WavpackContext *(*OpenFileInputEx)(void *,
	    void *, void *, char *, int, int);
    WavpackContext *(*CloseFile)(WavpackContext *);
    int (*GetMode)(WavpackContext *);
    int (*GetNumChannels)(WavpackContext *);
    uint32_t (*GetSampleRate)(WavpackContext *);
    int (*GetBitsPerSample)(WavpackContext *);
    uint32_t (*GetNumSamples)(WavpackContext *);
    int (*GetChannelMask)(WavpackContext *);
    int (*GetNumTagItems)(WavpackContext *);
    int (*GetTagItem)(WavpackContext *, const char *, char *, int);
    int (*GetTagItemIndexed)(WavpackContext *, int, char *, int);
    int (*SeekSample)(WavpackContext *, uint32_t);
    uint32_t (*UnpackSamples)(WavpackContext *, int32_t *, uint32_t);
};

class WavpackSource:
    public ITagParser, public PartialSource<WavpackSource>
{
    WavpackModule m_module;
    InputStream m_stream;
    x::shared_ptr<InputStream> m_cstream;
    SampleFormat m_format;
    x::shared_ptr<void> m_wpc;
    std::vector<uint32_t> m_chanmap;
    std::map<uint32_t, std::wstring> m_tags;
    std::vector<std::pair<std::wstring, int64_t> > m_chapters;
public:
    WavpackSource(const WavpackModule &module, InputStream &stream,
		  const std::wstring &path);
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannels() const { return &m_chanmap; }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const
    {
	if (m_chapters.size())
	    return &m_chapters;
	else
	    return 0;
    }
private:
    template <class MemorySink>
    size_t readSamplesT(void *buffer, size_t nsamples);
    void fetchTags();
};

#endif
