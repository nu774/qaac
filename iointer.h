#ifndef _IOINTER_H
#define _IOINTER_H

#include <vector>
#include <map>
#include <memory>
#include "CoreAudio/CoreAudioTypes.h"
#include "util.h"
#include "chapters.h"

struct ISource {
    virtual ~ISource() {}
    virtual uint64_t length() const = 0;
    virtual const AudioStreamBasicDescription &getSampleFormat() const = 0;
    virtual const std::vector<uint32_t> *getChannels() const = 0;
    virtual int64_t getPosition() = 0;
    virtual size_t readSamples(void *buffer, size_t nsamples) = 0;
};

struct ISeekableSource: public ISource {
    virtual bool isSeekable() = 0;
    virtual void seekTo(int64_t offset) = 0;
};

struct ISink {
    virtual ~ISink() {}
    virtual void writeSamples(
            const void *data, size_t len, size_t nsamples) = 0;
};

struct ITagParser {
    virtual ~ITagParser() {}
    virtual const std::map<std::string, std::string> &getTags() const = 0;
};

struct IChapterParser {
    virtual ~IChapterParser() {}
    virtual const std::vector<chapters::entry_t> &getChapters() const = 0;
};

struct ITagStore {
    virtual ~ITagStore() {}
    virtual void setTag(const std::string &key, const std::string &value) = 0;
};

class FilterBase: public ISource {
    std::shared_ptr<ISource> m_src;
public:
    FilterBase() {}
    FilterBase(const std::shared_ptr<ISource> &src): m_src(src) {}
    void setSource(const std::shared_ptr<ISource> &src) { m_src = src; }
    ISource *source() { return m_src.get(); }
    const std::shared_ptr<ISource> &sourcePtr() const { return m_src; }
    uint64_t length() const { return m_src->length(); }
    int64_t getPosition() { return m_src->getPosition(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_src->getSampleFormat();
    }
    const std::vector<uint32_t> *getChannels() const
    {
        return m_src->getChannels();
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
        return m_src->readSamples(buffer, nsamples);
    }
};

size_t readSamplesFull(ISource *src, void *buffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          std::vector<float> *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          float *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          std::vector<double> *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          double *floatBuffer, size_t nsamples);

#endif

