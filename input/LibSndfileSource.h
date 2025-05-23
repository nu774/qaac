#ifndef _LIBSNDFILESRC_H
#define _LIBSNDFILESRC_H

#include <vector>
#include <sndfile.h>
#include "ISource.h"
#include "dl.h"
#include "IInputStream.h"

struct HINSTANCE__;

class LibSndfileModule {
    DL m_dl;
private:
    LibSndfileModule() {
        load(L"sndfile.dll");
        if (!loaded()) load(L"libsndfile.dll");
        if (!loaded()) load(L"libsndfile-1.dll");
    }
    LibSndfileModule(const LibSndfileModule&);
    LibSndfileModule& operator=(const LibSndfileModule&);
public:
    static LibSndfileModule &instance() {
        static LibSndfileModule self;
        return self;
    }
    bool load(const std::wstring &path);
    bool loaded() const { return m_dl.loaded(); }

    const char *(*version_string)();
    SNDFILE *(*open_virtual)(SF_VIRTUAL_IO*, int, SF_INFO*, void*);
    int (*close)(SNDFILE *);
    const char *(*strerror)(SNDFILE *);
    int (*command)(SNDFILE *, int, void *, int);
    sf_count_t (*seek)(SNDFILE *, sf_count_t, int);
    /* XXX
     * cheat as void to avoid unnecessary type casting
     */
    sf_count_t (*readf_int)(SNDFILE *, void *, sf_count_t);
    sf_count_t (*readf_float)(SNDFILE *, void *, sf_count_t);
    sf_count_t (*readf_double)(SNDFILE *, void *, sf_count_t);
};

class LibSndfileSource: public ISeekableSource, public ITagParser
{
    typedef std::shared_ptr<SNDFILE_tag> handle_t;
    handle_t m_handle;
    uint64_t m_length;
    std::string m_format_name;
    std::shared_ptr<IInputStream> m_stream;
    std::vector<uint32_t> m_chanmap;
    std::map<std::string, std::string> m_tags;
    LibSndfileModule &m_module;
    AudioStreamBasicDescription m_asbd;
    sf_count_t (*m_readf)(SNDFILE *, void *, sf_count_t);
public:
    LibSndfileSource(std::shared_ptr<IInputStream> stream);
    ~LibSndfileSource() { m_handle.reset(); }
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
    size_t readSamples(void *buffer, size_t nsamples)
    {
        return static_cast<size_t>(m_readf(m_handle.get(), buffer, nsamples));
    }
    void seekTo(int64_t count);
    int64_t getPosition();
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
private:
    void fetchVorbisTags(int codec);
};

#endif
