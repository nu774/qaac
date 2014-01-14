#include <cstdio>
#include <cstring>
#include <cmath>
#include <float.h>
#include "normalize.h"
#ifdef _WIN32
#include "win32util.h"
#endif
#include "cautil.h"

Normalizer::Normalizer(const std::shared_ptr<ISource> &src, bool seekable)
    : FilterBase(src),
      m_peak(0.0),
      m_processed(0),
      m_position(0)
{
    const AudioStreamBasicDescription &asbd = source()->getSampleFormat();
    unsigned bits = 32;
    if (asbd.mBitsPerChannel > 32
        || (asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger) &&
           asbd.mBitsPerChannel > 24)
        bits = 64;

    m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate,
                                     asbd.mChannelsPerFrame,
                                     bits, kAudioFormatFlagIsFloat);
    if (!seekable) {
        FILE *tmpfile = win32::tmpfile(L"qaac.norm");
        m_tmpfile = std::shared_ptr<FILE>(tmpfile, std::fclose);
    }
}

size_t Normalizer::process(size_t nsamples)
{
    if (m_asbd.mBitsPerChannel == 32)
        return processT<float>(nsamples);
    else
        return processT<double>(nsamples);
}

size_t Normalizer::readSamples(void *buffer, size_t nsamples)
{
    if (m_asbd.mBitsPerChannel == 32)
        return readSamplesT<float>(buffer, nsamples);
    else
        return readSamplesT<double>(buffer, nsamples);
}

template <typename T>
size_t Normalizer::processT(size_t nsamples)
{
    if (m_fbuffer.size() < nsamples * m_asbd.mBytesPerFrame)
        m_fbuffer.resize(nsamples * m_asbd.mBytesPerFrame);
    T *bp = reinterpret_cast<T*>(&m_fbuffer[0]);
    size_t nc = readSamplesAsFloat(source(), &m_ibuffer, bp, nsamples);
    if (nc > 0) {
        m_processed += nc;
        if (fd() > 0)
            CHECKCRT(write(fd(), bp, nc * m_asbd.mBytesPerFrame) < 0);
        for (size_t i = 0; i < nc * m_asbd.mChannelsPerFrame; ++i) {
            double x = std::abs(bp[i]);
            if (x > m_peak) m_peak = x;
        }
    } else if (fd() > 0)
        CHECKCRT(_lseeki64(fd(), 0, SEEK_SET) < 0);
    return nc;
}

template <typename T>
size_t Normalizer::readSamplesT(void *buffer, size_t nsamples)
{
    if (!m_tmpfile.get())
        return 0;
    int nc = util::nread(fd(), buffer, nsamples * m_asbd.mBytesPerFrame);
    T *fp = static_cast<T*>(buffer);
    if (m_peak > FLT_MIN) {
        T peak = m_peak / 0.99609375;
        for (size_t i = 0; i < nc / sizeof(T); ++i)
            fp[i] = fp[i] / peak;
    }
    nsamples = std::max(0, static_cast<int>(nc/m_asbd.mBytesPerFrame));
    m_position += nsamples;
    return nsamples;
}
