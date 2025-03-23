#include <cstring>
#include <limits>
#include <assert.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "WaveSource.h"
#include "util.h"
#include "win32util.h"
#include "chanmap.h"

#define FOURCCR(a,b,c,d) ((a)|((b)<<8)|((c)<<16)|((d)<<24))

namespace wave {
    const GUID ksFormatSubTypePCM = {
        0x1, 0x0, 0x10, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
    };
    const GUID ksFormatSubTypeFloat = {
        0x3, 0x0, 0x10, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
    };
}

WaveSource::WaveSource(std::shared_ptr<IInputStream> stream, bool ignorelength)
    : m_data_pos(0), m_position(0), m_stream(stream)
{
    std::memset(&m_asbd, 0, sizeof m_asbd);
    int64_t data_length = parse();
    if (ignorelength || !data_length || data_length % m_block_align)
        m_length = ~0ULL;
    else
        m_length = data_length / m_block_align;
    m_data_pos = m_stream->tell();
    if (m_length == ~0ULL) {
        int64_t fsize = m_stream->size();
        if (fsize > 0)
            m_length = (fsize - m_data_pos) / m_block_align;
    }
}

size_t WaveSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_length != ~0ULL) {
        nsamples = static_cast<size_t>(std::min(static_cast<uint64_t>(nsamples),
                                                m_length - m_position));
    }
    ssize_t nbytes = nsamples * m_block_align;
    if (m_buffer.size() < nbytes)
        m_buffer.resize(nbytes);
    nbytes = m_stream->read(&m_buffer[0], nbytes);
    nsamples = nbytes > 0 ? nbytes / m_block_align: 0;
    if (nsamples) {
        size_t size = nsamples * m_block_align;
        util::unpack(&m_buffer[0], buffer, &size,
                     m_block_align / m_asbd.mChannelsPerFrame,
                     m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame);
        /* convert to signed */
        if (m_asbd.mBitsPerChannel <= 8) {
            util::convert_sign(static_cast<uint32_t *>(buffer),
                               nsamples * m_asbd.mChannelsPerFrame);
        }
        m_position += nsamples;
    }
    return nsamples;
}
void WaveSource::seekTo(int64_t count)
{

    CHECKCRT(m_stream->seek(m_data_pos + count * m_block_align, SEEK_SET) < 0);
    m_position = count;
}

int64_t WaveSource::parse()
{
    int64_t data_length = 0;

    uint32_t fcc = nextChunk(0);
    if (fcc != FOURCCR('R','I','F','F') && fcc != FOURCCR('R','F','6','4'))
        throw std::runtime_error("WaveSource: not a wav file");

    uint32_t wave;
    read32le(&wave);
    if (wave != FOURCCR('W','A','V','E'))
        throw std::runtime_error("WaveSource: not a wav file");

    if (fcc == FOURCCR('R','F','6','4'))
        data_length = ds64();

    uint32_t size;
    while (nextChunk(&size) != FOURCCR('f','m','t',' '))
        m_stream->seek((size + 1) & ~1, SEEK_CUR);
    fmt(size);

    while (nextChunk(&size) != FOURCCR('d','a','t','a'))
        m_stream->seek((size + 1) & ~1, SEEK_CUR);
    if (fcc != FOURCCR('R','F','6','4'))
        data_length = size;

    return data_length;
}

inline void WaveSource::read16le(void *n)
{
    util::check_eof(m_stream->read(n, 2) == 2);
}

inline void WaveSource::read32le(void *n)
{
    util::check_eof(m_stream->read(n, 4) == 4);
}

inline void WaveSource::read64le(void *n)
{
    util::check_eof(m_stream->read(n, 8) == 8);
}

uint32_t WaveSource::nextChunk(uint32_t *size)
{
    uint32_t fcc, n;
    read32le(&fcc);
    read32le(&n);
    if (size) *size = n;
    return fcc;
}

int64_t WaveSource::ds64()
{
    uint32_t size;
    int64_t data_length;

    if (nextChunk(&size)!= FOURCCR('d','s','6','4'))
        throw std::runtime_error("WaveSource: ds64 is expected in RF64 file");
    if (size != 28)
        throw std::runtime_error("WaveSource: RF64 with non empty chunk table "
                                 "is not supported");
    m_stream->seek(8, SEEK_CUR); // RIFF size
    read64le(&data_length);
    m_stream->seek(12, SEEK_CUR); // sample count + chunk table size
    return data_length;
}

void WaveSource::fmt(size_t size)
{
    uint16_t wFormatTag, nChannels, nBlockAlign, wBitsPerSample, cbSize;
    uint32_t nSamplesPerSec, nAvgBytesPerSec, dwChannelMask = 0;
    uint16_t wValidBitsPerSample;
    wave::GUID guid;
    bool isfloat = false;

    if (size < 16)
        throw std::runtime_error("WaveSource: fmt chunk too small");

    read16le(&wFormatTag);
    if (wFormatTag != 1 && wFormatTag != 3 && wFormatTag != 0xfffe)
        throw std::runtime_error("WaveSource: not supported wave file");
    if (wFormatTag == 3)
        isfloat = true;

    read16le(&nChannels);
    read32le(&nSamplesPerSec);
    read32le(&nAvgBytesPerSec);
    read16le(&nBlockAlign);
    read16le(&wBitsPerSample);
    wValidBitsPerSample = wBitsPerSample;
    if (wFormatTag != 0xfffe)
        m_stream->seek((size - 15) & ~1, SEEK_CUR);

    if (!nChannels || !nSamplesPerSec || !nAvgBytesPerSec || !nBlockAlign)
        throw std::runtime_error("WaveSource: invalid wave fmt");
    if (!wBitsPerSample || wBitsPerSample & 0x7)
        throw std::runtime_error("WaveSource: invalid wave fmt");
    if (nBlockAlign != nChannels * wBitsPerSample / 8)
        throw std::runtime_error("WaveSource: invalid wave fmt");
    if (nAvgBytesPerSec != nSamplesPerSec * nBlockAlign)
        throw std::runtime_error("WaveSource: invalid wave fmt");
    if (nChannels > 8)
        throw std::runtime_error("WaveSource: too many number of channels");

    if (wFormatTag == 0xfffe) {
        if (size < 40)
            throw std::runtime_error("WaveSource: fmt chunk too small");
        read16le(&cbSize);
        read16le(&wValidBitsPerSample);
        read32le(&dwChannelMask);
        if (dwChannelMask > 0 && util::bitcount(dwChannelMask) >= nChannels)
            m_chanmap = chanmap::getChannels(dwChannelMask, nChannels);

        util::check_eof(m_stream->read(&guid, sizeof guid) == sizeof guid);
        m_stream->seek((size - 39) & ~1, SEEK_CUR);

        if (!std::memcmp(&guid, &wave::ksFormatSubTypeFloat, sizeof guid))
            isfloat = true;
        else if (std::memcmp(&guid, &wave::ksFormatSubTypePCM, sizeof guid))
            throw std::runtime_error("WaveSource: not supported wave file");

        if (!wValidBitsPerSample || wValidBitsPerSample > wBitsPerSample)
            throw std::runtime_error("WaveSource: invalid wave fmt");
    }

    if (isfloat) {
        if (wValidBitsPerSample != 16 && wValidBitsPerSample != 24 &&
            wValidBitsPerSample != 32 && wValidBitsPerSample != 64)
            throw std::runtime_error("WaveSource: not supported float format");
        if (wBitsPerSample > 64)
            throw std::runtime_error("WaveSource: not supported float format");
    } else if (wBitsPerSample > 32)
        throw std::runtime_error("WaveSource: not supported integer format");

    m_block_align = nBlockAlign;

    unsigned bits = 32;
    if (isfloat && wValidBitsPerSample > 32) bits = 64;
    else if (isfloat && wValidBitsPerSample <= 16) bits = 16;

    m_asbd = cautil::buildASBDForPCM2(nSamplesPerSec, nChannels,
                                      wValidBitsPerSample, bits,
                                      isfloat ? kAudioFormatFlagIsFloat
                                        : kAudioFormatFlagIsSignedInteger);
}
