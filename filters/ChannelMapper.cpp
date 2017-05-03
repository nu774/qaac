#include <cassert>
#include "ChannelMapper.h"

ChannelMapper::ChannelMapper(const std::shared_ptr<ISource> &source,
                             const std::vector<uint32_t> &chanmap,
                             uint32_t bitmap)
    : FilterBase(source)
{
    const AudioStreamBasicDescription &asbd = source->getSampleFormat();
    assert(chanmap.size() == asbd.mChannelsPerFrame);
    assert(chanmap.size() <= 8);

    for (size_t i = 0; i < chanmap.size(); ++i)
        m_chanmap.push_back(chanmap[i] - 1);
    if (bitmap) {
        for (unsigned i = 0; i < 32; ++i, bitmap >>= 1)
            if (bitmap & 1) m_layout.push_back(i + 1);
    } else {
        const std::vector<uint32_t> *orig = FilterBase::getChannels();
        if (orig)
            for (size_t i = 0; i < m_chanmap.size(); ++i)
                m_layout.push_back(orig->at(m_chanmap[i]));
    }
    switch (asbd.mBytesPerFrame / asbd.mChannelsPerFrame) {
    case 2:
        m_process = &ChannelMapper::process16; break;
    case 4:
        m_process = &ChannelMapper::process32; break;
    case 8:
        m_process = &ChannelMapper::process64; break;
    default:
        assert(0);
    }
}

template <typename T>
size_t ChannelMapper::processT(T *buffer, size_t nsamples)
{
    unsigned nchannels = source()->getSampleFormat().mChannelsPerFrame;
    const uint32_t *chanmap = &m_chanmap[0];
    T work[8], *bp = buffer;

    nsamples = source()->readSamples(buffer, nsamples);
    for (size_t i = 0; i < nsamples; ++i, bp += nchannels) {
        memcpy(work, bp, sizeof(T) * nchannels);
        switch (nchannels) {
        case 8: bp[7] = work[chanmap[7]];
        case 7: bp[6] = work[chanmap[6]];
        case 6: bp[5] = work[chanmap[5]];
        case 5: bp[4] = work[chanmap[4]];
        case 4: bp[3] = work[chanmap[3]];
        case 3: bp[2] = work[chanmap[2]];
        case 2: bp[1] = work[chanmap[1]];
        case 1: bp[0] = work[chanmap[0]];
        }
    }
    return nsamples;
}

size_t ChannelMapper::process16(void *buffer, size_t nsamples)
{
    return processT(static_cast<uint16_t *>(buffer), nsamples);
}

size_t ChannelMapper::process32(void *buffer, size_t nsamples)
{
    return processT(static_cast<uint32_t *>(buffer), nsamples);
}

size_t ChannelMapper::process64(void *buffer, size_t nsamples)
{
    return processT(static_cast<uint64_t *>(buffer), nsamples);
}
