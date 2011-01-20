#ifndef _WVPACKSRC_H
#define _WVPACKSRC_H

#include "channel.h"
#include "iointer.h"

struct HINSTANCE__;
/*
 * XXX
 * wavpack.h conflicts with QT header, therefore we don't want to
 * include it here.
 */
typedef void WavpackContext;

class WavpackModule {
    typedef boost::shared_ptr<HINSTANCE__> module_t;
    module_t m_module;
    bool m_loaded;
public:
    WavpackModule(): m_loaded(false) {}
    explicit WavpackModule(const std::wstring &path);
    bool loaded() const { return m_loaded; }

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
    public ISource, public ITagParser, public PartialSource<WavpackSource>
{
    WavpackModule m_module;
    InputStream m_stream;
    SampleFormat m_format;
    boost::shared_ptr<void> m_wpc;
    std::vector<uint32_t> m_chanmap;
    std::map<uint32_t, std::wstring> m_tags;
    std::vector<std::pair<std::wstring, int64_t> > m_chapters;
public:
    WavpackSource(const WavpackModule &module, InputStream &stream);
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const { return &m_chanmap; }
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
    static int32_t f_read(void *cookie, void *data, int32_t count)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->read(data, count);
    }
    static uint32_t f_tell(void *cookie)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return static_cast<uint32_t>(pT->tell());
    }
    static int f_seek_abs(void *cookie, uint32_t pos)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->seek(pos, ISeekable::kBegin) >= 0 ? 0 : -1;
    }
    static int f_seek(void *cookie, int32_t off, int whence)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->seek(off, whence) >= 0 ? 0 : -1;
    }
    static int f_pushback(void *cookie, int c)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	pT->pushback(c);
	return c;
    }
    static uint32_t f_size(void *cookie)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	int64_t curpos = pT->tell();
	int32_t size = static_cast<int32_t>(pT->seek(0, ISeekable::kEnd));
	pT->seek(curpos, ISeekable::kBegin);
	return size;
    }
    static int f_seekable(void *cookie)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->seekable();
    }
    static int32_t f_write(void *, void *, int32_t) { return -1; }
};

#endif
