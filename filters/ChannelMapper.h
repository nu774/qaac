#ifndef _CHANNELMAPPER_H
#define _CHANNELMAPPER_H

#include "FilterBase.h"

class ChannelMapper: public FilterBase {
    std::vector<uint32_t> m_chanmap;
    std::vector<uint32_t> m_layout;
    size_t (ChannelMapper::*m_process)(void *, size_t);
public:
    ChannelMapper(const std::shared_ptr<ISource> &source,
                  const std::vector<uint32_t> &chanmap, uint32_t bitmap=0);
    const std::vector<uint32_t> *getChannels() const
    {
        return m_layout.size() ? &m_layout : 0;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
        return (this->*m_process)(buffer, nsamples);
    }
private:
    template <typename T>
    size_t processT(T *buffer, size_t nsamples);
    size_t process16(void *buffer, size_t nsamples);
    size_t process32(void *buffer, size_t nsamples);
    size_t process64(void *buffer, size_t nsamples);
};

#endif
