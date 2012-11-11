#include <cstdio>
#include "iointer.h"

inline float quantize(double v)
{
    const float anti_denormal = 1.0e-30f;
    float x = static_cast<float>(v);
    x += anti_denormal;
    x -= anti_denormal;
    return x;
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

    if ((sf.mFormatFlags & kAudioFormatFlagIsFloat) &&
	sf.mBytesPerFrame / sf.mChannelsPerFrame == 4)
    {
	return src->readSamples(floatBuffer, nsamples);
    }

    if (pivot->size() < nsamples * sf.mBytesPerFrame)
	pivot->resize(nsamples * sf.mBytesPerFrame);

    uint8_t *bp = &(*pivot)[0];
    float *fp = floatBuffer;
    nsamples = src->readSamples(bp, nsamples);
    size_t blen = nsamples * sf.mBytesPerFrame;

    if (sf.mFormatFlags & kAudioFormatFlagIsFloat) {
	double *src = reinterpret_cast<double *>(bp);
	std::transform(src, src + (blen / 8), fp, quantize);
    } else {
	int *src = reinterpret_cast<int *>(bp);
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

    if ((sf.mFormatFlags & kAudioFormatFlagIsFloat) &&
	sf.mBytesPerFrame / sf.mChannelsPerFrame == 8)
    {
	return src->readSamples(doubleBuffer, nsamples);
    }

    if (pivot->size() < nsamples * sf.mBytesPerFrame)
	pivot->resize(nsamples * sf.mBytesPerFrame);

    uint8_t *bp = &(*pivot)[0];
    double *fp = doubleBuffer;
    nsamples = src->readSamples(bp, nsamples);
    size_t blen = nsamples * sf.mBytesPerFrame;

    if (sf.mFormatFlags & kAudioFormatFlagIsFloat) {
	float *src = reinterpret_cast<float*>(bp);
	std::copy(src, src + (blen / 4), fp);
    } else {
	int *src = reinterpret_cast<int *>(bp);
	for (size_t i = 0; i < blen / 4; ++i)
	    *fp++ = src[i] / 2147483648.0f;
    }
    return nsamples;
}

