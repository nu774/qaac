#include <cstdio>
#include "iointer.h"

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *byteBuffer,
			  std::vector<float> *floatBuffer, size_t nsamples)
{
    const AudioStreamBasicDescription &sf = src->getSampleFormat();
    if (floatBuffer->size() < nsamples * sf.mChannelsPerFrame)
	floatBuffer->resize(nsamples * sf.mChannelsPerFrame);
    return readSamplesAsFloat(src, byteBuffer, &floatBuffer->at(0), nsamples);
}

inline float quantize(double v)
{
    const float anti_denormal = 1.0e-30f;
    float x = static_cast<float>(v);
    x += anti_denormal;
    x -= anti_denormal;
    return x;
}

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *byteBuffer,
			  float *floatBuffer, size_t nsamples)
{
    const AudioStreamBasicDescription &sf = src->getSampleFormat();
    if (byteBuffer->size() < nsamples * sf.mBytesPerFrame)
	byteBuffer->resize(nsamples * sf.mBytesPerFrame);

    uint8_t *bp = &(*byteBuffer)[0];
    float *fp = floatBuffer;
    nsamples = src->readSamples(bp, nsamples);
    size_t blen = nsamples * sf.mBytesPerFrame;

    if (sf.mFormatFlags & kAudioFormatFlagIsFloat) {
	switch (sf.mBytesPerFrame / sf.mChannelsPerFrame) {
	case 4:
	    {
		if (sf.mFormatFlags & kAudioFormatFlagIsBigEndian)
		    util::bswap32buffer(bp, blen);
		std::memcpy(fp, bp, blen);
	    }
	    break;
	case 8:
	    {
		if (sf.mFormatFlags & kAudioFormatFlagIsBigEndian)
		    util::bswap64buffer(bp, blen);
		double *src = reinterpret_cast<double *>(bp);
		std::transform(src, src + (blen >> 3), fp, quantize);
	    }
	    break;
	}
    } else {
	switch (sf.mBytesPerFrame / sf.mChannelsPerFrame) {
	case 1:
	    {
		if (!(sf.mFormatFlags & kAudioFormatFlagIsSignedInteger))
		    for (size_t i = 0; i < blen; ++i)
			bp[i] ^= 0x80;
		char *src = reinterpret_cast<char *>(bp);
		for (size_t i = 0; i < blen; ++i)
		    *fp++ = static_cast<float>(src[i]) / 0x80;
	    }
	    break;
	case 2:
	    {
		if (sf.mFormatFlags & kAudioFormatFlagIsBigEndian)
		    util::bswap16buffer(bp, blen);
		short *src = reinterpret_cast<short *>(bp);
		for (size_t i = 0; i < blen >> 1; ++i)
		    *fp++ = static_cast<float>(src[i]) / 0x8000;
	    }
	    break;
	case 3:
	    {
		if (sf.mFormatFlags & kAudioFormatFlagIsBigEndian)
		    util::bswap24buffer(bp, blen);
		for (size_t i = 0; i < blen / 3; ++i) {
		    int32_t v = bp[i*3]<<8 | bp[i*3+1]<<16 | bp[i*3+2]<<24;
		    *fp++ = static_cast<float>(v) / 0x80000000U;
		}
	    }
	    break;
	case 4:
	    {
		if (sf.mFormatFlags & kAudioFormatFlagIsBigEndian)
		    util::bswap32buffer(bp, blen);
		int *src = reinterpret_cast<int *>(bp);
		for (size_t i = 0; i < blen >> 2; ++i)
		    *fp++ = static_cast<float>(src[i]) / 0x80000000U;
	    }
	    break;
	}
    }
    return nsamples;
}

