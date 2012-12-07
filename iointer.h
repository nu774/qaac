#ifndef _IOINTER_H
#define _IOINTER_H

#include <vector>
#include <map>
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
    virtual const std::map<uint32_t, std::wstring> &getTags() const = 0;
    virtual const std::vector<chapters::entry_t> *getChapters() const = 0;
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

/*
 * Kind of a simple FIFO, but only applicable for iterations of
 * put once -> get N times, where all items are consumed on each iteration.
 */
template <typename T>
class DecodeBuffer {
    uint32_t npackets_;
    uint32_t position_;
    std::vector<T> v_;
public:
    uint32_t units_per_packet; // bytes per frame, or number of channels

    DecodeBuffer(): npackets_(0), position_(0), units_per_packet(0) {}
    void resize(uint32_t npackets)
    {
        size_t n = npackets * units_per_packet;
        if (n > v_.size()) v_.resize(n);
    }
    T *read_ptr() { return &v_[position_ * units_per_packet]; }
    T *write_ptr() { return &v_[0]; }
    void reset() { npackets_ = position_ = 0; }
    uint32_t count() { return npackets_ - position_; }
    void advance(uint32_t n) {
        position_ += n;
        if (position_ >= npackets_)
            reset();
    }
    void commit(uint32_t count)
    {
        position_ = 0;
        npackets_ = count;
    }
};

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          std::vector<float> *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          float *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          std::vector<double> *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          double *floatBuffer, size_t nsamples);

namespace chapters {
    struct Track {
        std::wstring name;
        std::shared_ptr<ISeekableSource> source;
        std::wstring ofilename;
    };
};

#endif

