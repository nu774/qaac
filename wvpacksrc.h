#ifndef _WVPACKSRC_H
#define _WVPACKSRC_H

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
    WavpackContext *(*OpenFileInputEx)(void *, void *, void *, char *, int,
                                       int);
    WavpackContext *(*CloseFile)(WavpackContext *);
    int (*GetBitsPerSample)(WavpackContext *);
    int (*GetChannelMask)(WavpackContext *);
    int (*GetMode)(WavpackContext *);
    int (*GetNumChannels)(WavpackContext *);
    uint32_t (*GetNumSamples)(WavpackContext *);
    int (*GetNumTagItems)(WavpackContext *);
    int (*GetNumBinaryTagItems)(WavpackContext *);
    uint32_t (*GetSampleIndex)(WavpackContext *);
    uint32_t (*GetSampleRate)(WavpackContext *);
    int (*GetTagItem)(WavpackContext *, const char *, char *, int);
    int (*GetBinaryTagItem)(WavpackContext *, const char *, char *, int);
    int (*GetTagItemIndexed)(WavpackContext *, int, char *, int);
    int (*GetBinaryTagItemIndexed)(WavpackContext *, int, char *, int);
    void *(*GetWrapperLocation)(void *first_block, uint32_t *size);
    int (*SeekSample)(WavpackContext *, uint32_t);
    uint32_t (*UnpackSamples)(WavpackContext *, int32_t *, uint32_t);
};

class WavpackSource: public ISeekableSource, public ITagParser
{
    uint64_t m_length;
    std::shared_ptr<void> m_wpc;
    std::shared_ptr<FILE> m_fp, m_cfp;
    std::vector<uint32_t> m_chanmap;
    std::map<std::string, std::string> m_tags;
    std::vector<uint8_t> m_pivot;
    size_t (WavpackSource::*m_readSamples)(void *, size_t);
    AudioStreamBasicDescription m_asbd;
    WavpackModule m_module;
public:
    WavpackSource(const WavpackModule &module, const std::wstring &path);
    ~WavpackSource() { m_wpc.reset(); }
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
        return m_chanmap.size() ? &m_chanmap : 0;
    }
    int64_t getPosition();
    size_t readSamples(void *buffer, size_t nsamples)
    {
        return (this->*m_readSamples)(buffer, nsamples);
    }
    bool isSeekable() { return win32::is_seekable(fileno(m_fp.get())); }
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
private:
    bool parseWrapper();
    void fetchTags();
    size_t readSamples16(void *buffer, size_t nsamples);
    size_t readSamples32(void *buffer, size_t nsamples);
};

#endif
