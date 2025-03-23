#include "RawSource.h"
#include "win32util.h"
#include "cautil.h"

RawSource::RawSource(std::shared_ptr<IInputStream> stream,
                     const AudioStreamBasicDescription &asbd)
    : m_position(0), m_stream(stream), m_asbd(asbd)
{
    int64_t file_size = m_stream->size();
    if (file_size >= 0)
        m_length = file_size / asbd.mBytesPerFrame;
    else
        m_length = ~0ULL;
    bool isfloat = asbd.mFormatFlags & kAudioFormatFlagIsFloat;
    m_oasbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
                                       asbd.mChannelsPerFrame,
                                       asbd.mBitsPerChannel,
                                       isfloat ? asbd.mBitsPerChannel : 32,
                                       isfloat ? kAudioFormatFlagIsFloat
                                          : kAudioFormatFlagIsSignedInteger);
}

size_t RawSource::readSamples(void *buffer, size_t nsamples)
{
    ssize_t nbytes = nsamples * m_asbd.mBytesPerFrame;
    if (m_buffer.size() < nbytes)
        m_buffer.resize(nbytes);
    nbytes = m_stream->read(&m_buffer[0], nbytes);
    nsamples = nbytes > 0 ? nbytes / m_asbd.mBytesPerFrame : 0;
    if (nsamples) {
        size_t size = nsamples * m_asbd.mBytesPerFrame;

        /* bswap */
        if (m_asbd.mFormatFlags & kAudioFormatFlagIsBigEndian)
            util::bswapbuffer(&m_buffer[0], size,
                              (m_asbd.mBitsPerChannel + 7) & ~7);

        util::unpack(&m_buffer[0], buffer, &size,
                     m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame,
                     m_oasbd.mBytesPerFrame / m_oasbd.mChannelsPerFrame);
        /* convert to signed */
        if (!(m_asbd.mFormatFlags & kAudioFormatFlagIsFloat) &&
            !(m_asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger))
        {
            util::convert_sign(static_cast<uint32_t *>(buffer),
                               nsamples * m_asbd.mChannelsPerFrame);
        }
    }
    m_position += nsamples;
    return nsamples;
}

void RawSource::seekTo(int64_t count)
{
    CHECKCRT(m_stream->seek(count*m_asbd.mBytesPerFrame, SEEK_SET) < 0);
    m_position = count;
}
