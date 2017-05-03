#include <cstdio>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "ISource.h"

namespace {
    static union uif_t {
        uint32_t i;
        float    f;
    } *h2s_table;

    inline float quantize(double v)
    {
        const float anti_denormal = 1.0e-30f;
        float x = static_cast<float>(v);
        x += anti_denormal;
        x -= anti_denormal;
        return x;
    }

    uint32_t half2single_(uint16_t n)
    {
        unsigned sign = n >> 15;
        unsigned exp  = (n >> 10) & 0x1F;
        unsigned mantissa = n & 0x3FF;

        if (exp == 0 && mantissa == 0)
            return sign << 31;
        if (exp == 0x1F)
            exp = 0x8F;
        else if (exp == 0) {
            for (; !(mantissa & 0x400); mantissa <<= 1, --exp)
                ;
            ++exp;
            mantissa &= ~0x400;
        }
        return (sign << 31) | ((exp + 0x70) << 23) | (mantissa << 13);
    }
    void init_h2s_table()
    {
        if (!h2s_table) {
            uif_t *p = new uif_t[1<<16];
            for (unsigned n = 0; n < (1<<16); ++n)
                p[n].i = half2single_(n);
            InterlockedCompareExchangePointerRelease((void**)&h2s_table, p, 0);
        }
    }
}

size_t readSamplesFull(ISource *src, void *buffer, size_t nsamples)
{
    uint8_t *bp = static_cast<uint8_t*>(buffer);
    size_t n, rest = nsamples;
    unsigned bpf = src->getSampleFormat().mBytesPerFrame;
    while (rest > 0 && (n = src->readSamples(bp, rest)) > 0) {
        rest -= n;
        bp += n * bpf;
    }
    return nsamples - rest;
}

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          std::vector<float> *floatBuffer, size_t nsamples)
{
    const AudioStreamBasicDescription &sf = src->getSampleFormat();
    if (floatBuffer->size() < nsamples * sf.mChannelsPerFrame)
        floatBuffer->resize(nsamples * sf.mChannelsPerFrame);
    return readSamplesAsFloat(src, pivot, &(*floatBuffer)[0], nsamples);
}

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          float *floatBuffer, size_t nsamples)
{
    const AudioStreamBasicDescription &sf = src->getSampleFormat();
    uint32_t bpc = sf.mBytesPerFrame / sf.mChannelsPerFrame;

    if ((sf.mFormatFlags & kAudioFormatFlagIsFloat) && bpc == 4)
        return src->readSamples(floatBuffer, nsamples);

    if (pivot->size() < nsamples * sf.mBytesPerFrame)
        pivot->resize(nsamples * sf.mBytesPerFrame);

    void *bp = &(*pivot)[0];
    float *fp = floatBuffer;
    nsamples = src->readSamples(bp, nsamples);
    size_t blen = nsamples * sf.mBytesPerFrame;

    if (sf.mFormatFlags & kAudioFormatFlagIsFloat) {
        if (bpc == 8) {
            double *src = static_cast<double *>(bp);
            std::transform(src, src + (blen / 8), fp, quantize);
        } else if (bpc == 2) {
            uint16_t *src = static_cast<uint16_t *>(bp);
            init_h2s_table();
            for (size_t i = 0; i < blen / 2; ++i)
                *fp++ = h2s_table[src[i]].f / 65536.0;
        } else {
            throw std::runtime_error("readSamplesAsFloat(): BUG");
        }
    } else {
        int *src = static_cast<int *>(bp);
        for (size_t i = 0; i < blen / 4; ++i)
            *fp++ = src[i] / 2147483648.0f;
    }
    return nsamples;
}

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          std::vector<double> *doubleBuffer, size_t nsamples)
{
    const AudioStreamBasicDescription &sf = src->getSampleFormat();
    if (doubleBuffer->size() < nsamples * sf.mChannelsPerFrame)
        doubleBuffer->resize(nsamples * sf.mChannelsPerFrame);
    return readSamplesAsFloat(src, pivot, &(*doubleBuffer)[0], nsamples);
}

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          double *doubleBuffer, size_t nsamples)
{
    const AudioStreamBasicDescription &sf = src->getSampleFormat();
    uint32_t bpc = sf.mBytesPerFrame / sf.mChannelsPerFrame;

    if ((sf.mFormatFlags & kAudioFormatFlagIsFloat) && bpc == 8)
        return src->readSamples(doubleBuffer, nsamples);

    if (pivot->size() < nsamples * sf.mBytesPerFrame)
        pivot->resize(nsamples * sf.mBytesPerFrame);

    void *bp = &(*pivot)[0];
    double *fp = doubleBuffer;
    nsamples = src->readSamples(bp, nsamples);
    size_t blen = nsamples * sf.mBytesPerFrame;

    if (sf.mFormatFlags & kAudioFormatFlagIsFloat) {
        if (bpc == 4) {
            float *src = static_cast<float*>(bp);
            std::copy(src, src + (blen / 4), fp);
        } else if (bpc == 2) {
            uint16_t *src = static_cast<uint16_t *>(bp);
            init_h2s_table();
            for (size_t i = 0; i < blen / 2; ++i) {
                *fp++ = h2s_table[src[i]].f / 65536.0;
            }
        } else {
            throw std::runtime_error("readSamplesAsFloat(): BUG");
        }
    } else {
        int *src = static_cast<int *>(bp);
        for (size_t i = 0; i < blen / 4; ++i)
            *fp++ = src[i] / 2147483648.0;
    }
    return nsamples;
}

