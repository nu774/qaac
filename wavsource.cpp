#include <cstring>
#include <limits>
#include <assert.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "wavsource.h"
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

WaveSource::WaveSource(const std::shared_ptr<FILE> &fp, bool ignorelength)
    : m_data_pos(0), m_position(0), m_fp(fp)
{
    std::memset(&m_asbd, 0, sizeof m_asbd);
    m_seekable = util::is_seekable(fileno(m_fp.get()));
    int64_t data_length = parse();
    if (ignorelength || !data_length || data_length % m_block_align)
        m_length = ~0ULL;
    else
        m_length = data_length / m_block_align;
    if (m_seekable)
        m_data_pos = _lseeki64(fd(), 0, SEEK_CUR);
}

size_t WaveSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_length != ~0ULL) {
        nsamples = static_cast<size_t>(std::min(static_cast<uint64_t>(nsamples),
                                                m_length - m_position));
    }
    ssize_t nbytes = nsamples * m_block_align;
    m_buffer.resize(nbytes);
    nbytes = util::nread(fd(), &m_buffer[0], nbytes);
    nsamples = nbytes > 0 ? nbytes / m_block_align: 0;
    if (nsamples) {
        size_t size = nsamples * m_block_align;
        uint8_t *bp = &m_buffer[0];
        /* convert to signed */
        if (m_asbd.mBitsPerChannel <= 8) {
            for (size_t i = 0; i < size; ++i)
                bp[i] ^= 0x80;
        }
        util::unpack(bp, buffer, &size,
                     m_block_align / m_asbd.mChannelsPerFrame,
                     m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame);
        m_position += nsamples;
    }
    return nsamples;
}
void WaveSource::seekTo(int64_t count)
{
    if (m_seekable) {
        CHECKCRT(_lseeki64(fd(), m_data_pos + count * m_block_align,
                           SEEK_SET) < 0);
        m_position = count;
    }
    else if (m_position > count)
        throw std::runtime_error("Cannot seek back the input");
    else {
        char buf[0x1000];
        int64_t nread = 0;
        int64_t bytes = (count - m_position) * m_block_align;
        while (nread < bytes) {
            int n = util::nread(fd(), buf, std::min(bytes - nread, 0x1000LL));
            if (n < 0) break;
            nread += n;
        }
        m_position += nread / m_block_align;
    }
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
        skip((size + 1) & ~1);
    fmt(size);

    while (nextChunk(&size) != FOURCCR('d','a','t','a'))
        skip((size + 1) & ~1);
    if (fcc != FOURCCR('R','F','6','4'))
        data_length = size;

    return data_length;
}

inline void WaveSource::read16le(void *n)
{
    util::check_eof(util::nread(fd(), n, 2) == 2);
}

inline void WaveSource::read32le(void *n)
{
    util::check_eof(util::nread(fd(), n, 4) == 4);
}

inline void WaveSource::read64le(void *n)
{
    util::check_eof(util::nread(fd(), n, 8) == 8);
}

void WaveSource::skip(int64_t n)
{
    if (m_seekable)
        CHECKCRT(_lseeki64(fd(), n, SEEK_CUR) < 0);
    else {
        char buf[8192];
        while (n > 0) {
            int nn = static_cast<int>(std::min(n, 8192LL));
            util::check_eof(util::nread(fd(), buf, nn) == nn);
            n -= nn;
        }
    }
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
    skip(8); // RIFF size
    read64le(&data_length);
    skip(12); // sample count + chunk table size
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
        skip((size - 15) & ~1);

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
            chanmap::getChannels(dwChannelMask, &m_chanmap, nChannels);

        util::check_eof(util::nread(fd(), &guid, sizeof guid) == sizeof guid);
        skip((size - 39) & ~1);

        if (!std::memcmp(&guid, &wave::ksFormatSubTypeFloat, sizeof guid))
            isfloat = true;
        else if (std::memcmp(&guid, &wave::ksFormatSubTypePCM, sizeof guid))
            throw std::runtime_error("WaveSource: not supported wave file");

        if (!wValidBitsPerSample || wValidBitsPerSample > wBitsPerSample)
            throw std::runtime_error("WaveSource: invalid wave fmt");
    }

    m_block_align = nBlockAlign;
    m_asbd = cautil::buildASBDForPCM2(nSamplesPerSec, nChannels,
                                      wValidBitsPerSample,
                                      isfloat ? wBitsPerSample : 32,
                                      isfloat ? kAudioFormatFlagIsFloat
                                        : kAudioFormatFlagIsSignedInteger);
}
