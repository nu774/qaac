#ifndef _FILTERBASE_H
#define _FILTERBASE_H

#include "ISource.h"

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

#endif

