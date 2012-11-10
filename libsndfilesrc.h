#ifndef _LIBSNDFILESRC_H
#define _LIBSNDFILESRC_H

#include <vector>
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
    SNDFILE *(*open_virtual)(SF_VIRTUAL_IO*, int, SF_INFO*, void*);
    int (*close)(SNDFILE *);
    const char *(*strerror)(SNDFILE *);
    int (*command)(SNDFILE *, int, void *, int);
    sf_count_t (*seek)(SNDFILE *, sf_count_t, int);
    sf_count_t (*read_short)(SNDFILE *, short *, sf_count_t);
    sf_count_t (*read_int)(SNDFILE *, int *, sf_count_t);
    sf_count_t (*read_float)(SNDFILE *, float *, sf_count_t);
    sf_count_t (*read_double)(SNDFILE *, double *, sf_count_t);
};

class LibSndfileSource: public ISeekableSource, public ITagParser
{
    typedef std::shared_ptr<SNDFILE_tag> handle_t;
    handle_t m_handle;
    uint8_t m_length;
    std::string m_format_name;
    std::shared_ptr<FILE> m_fp;
    std::vector<uint32_t> m_chanmap;
    std::map<uint32_t, std::wstring> m_tags;
    LibSndfileModule m_module;
    AudioStreamBasicDescription m_asbd;
public:
    LibSndfileSource(const LibSndfileModule &module,
		     const std::shared_ptr<FILE> &fp);
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    const std::string &getFormatName() const { return m_format_name; }
    const std::vector<uint32_t> *getChannels() const
    {
	return m_chanmap.size() ? &m_chanmap: 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    bool isSeekable() { return util::is_seekable(fileno(m_fp.get())); }
    void seekTo(int64_t count);
    int64_t getPosition();
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<chapters::entry_t> *getChapters() const
    {
	return 0;
    }
private:
    size_t readSamples8(void *buffer, size_t nsamples);
    size_t readSamples24(void *buffer, size_t nsamples);
};

#endif
