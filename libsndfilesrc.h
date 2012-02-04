#ifndef _LIBSNDFILESRC_H
#define _LIBSNDFILESRC_H

#include <vector>
#include "shared_ptr.h"
#include <sndfile.h>
#include "iointer.h"
#include "dl.h"

struct HINSTANCE__;

class LibSndfileModule {
    DL m_dl;
public:
    LibSndfileModule() {}
    LibSndfileModule(const std::wstring &path);
    bool loaded() const { return m_dl.loaded(); }

    const char *(*version_string)();
    SNDFILE *(*wchar_open)(const wchar_t *, int, SF_INFO *);
    SNDFILE *(*open_fd)(int, int, SF_INFO *, int);
    int (*close)(SNDFILE *);
    const char *(*strerror)(SNDFILE *);
    int (*command)(SNDFILE *, int, void *, int);
    sf_count_t (*seek)(SNDFILE *, sf_count_t, int);
    sf_count_t (*read_short)(SNDFILE *, short *, sf_count_t);
    sf_count_t (*read_int)(SNDFILE *, int *, sf_count_t);
    sf_count_t (*read_float)(SNDFILE *, float *, sf_count_t);
    sf_count_t (*read_double)(SNDFILE *, double *, sf_count_t);
};

class LibSndfileSource:
    public ITagParser, public PartialSource<LibSndfileSource>
{
    typedef x::shared_ptr<SNDFILE_tag> handle_t;
    handle_t m_handle;
    LibSndfileModule m_module;
    SampleFormat m_format;
    std::vector<uint32_t> m_chanmap;
    std::string m_format_name;
    std::map<uint32_t, std::wstring> m_tags;
public:
    LibSndfileSource(const LibSndfileModule &module, const wchar_t *path);
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::string &getFormatName() const { return m_format_name; }
    const std::vector<uint32_t> *getChannels() const
    {
	return m_chanmap.size() ? &m_chanmap: 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const
    {
	    return 0;
    }
private:
    size_t readSamples8(void *buffer, size_t nsamples);
    size_t readSamples24(void *buffer, size_t nsamples);
};

#endif
