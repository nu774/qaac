#include <cstdio>
#include "iointer.h"

static
void die() { throw std::runtime_error("Invalid sample format"); }

SampleFormat::SampleFormat(const char *spec, unsigned nchannels, unsigned rate)
    : m_nchannels(nchannels), m_rate(rate)
{
    char c_type, c_endian;
    if (m_nchannels == 0 || m_nchannels > 8 || m_rate == 0)
	die();
    if (std::sscanf(spec, "%c%d%c",
		&c_type, &m_bitsPerSample, &c_endian) != 3)
	die();
    if ((m_type = strindex("SUF", toupper(c_type & 0xff))) == -1)
	die();
    if ((m_endian = strindex("LB", toupper(c_endian & 0xff))) == -1)
	die();
    if (!m_bitsPerSample || (m_bitsPerSample & 0x7)) die();
    if (m_type == kIsFloat && (m_bitsPerSample & 0x1f)) die();
}

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *byteBuffer,
			  std::vector<float> *floatBuffer, size_t nsamples)
{
    const SampleFormat &sf = src->getSampleFormat();
    if (floatBuffer->size() < nsamples * sf.m_nchannels)
	floatBuffer->resize(nsamples * sf.m_nchannels);
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
    const SampleFormat &sf = src->getSampleFormat();
    if (byteBuffer->size() < nsamples * sf.bytesPerFrame())
	byteBuffer->resize(nsamples * sf.bytesPerFrame());

    uint8_t *bp = &(*byteBuffer)[0];
    float *fp = floatBuffer;
    nsamples = src->readSamples(bp, nsamples);
    size_t blen = nsamples * sf.bytesPerFrame();

    if (sf.m_type == SampleFormat::kIsFloat) {
	switch (sf.m_bitsPerSample) {
	case 32:
	    {
		if (sf.m_endian == SampleFormat::kIsBigEndian)
		    bswap32buffer(bp, blen);
		std::memcpy(fp, bp, blen);
	    }
	    break;
	case 64:
	    {
		if (sf.m_endian == SampleFormat::kIsBigEndian)
		    bswap64buffer(bp, blen);
		double *src = reinterpret_cast<double *>(bp);
		std::transform(src, src + (blen >> 3), fp, quantize);
	    }
	    break;
	}
    } else {
	switch (sf.m_bitsPerSample) {
	case 8:
	    {
		if (sf.m_type == SampleFormat::kIsUnsignedInteger)
		    for (size_t i = 0; i < blen; ++i)
			bp[i] ^= 0x80;
		char *src = reinterpret_cast<char *>(bp);
		for (size_t i = 0; i < blen; ++i)
		    *fp++ = static_cast<float>(src[i]) / 0x80;
	    }
	    break;
	case 16:
	    {
		if (sf.m_endian == SampleFormat::kIsBigEndian)
		    bswap16buffer(bp, blen);
		short *src = reinterpret_cast<short *>(bp);
		for (size_t i = 0; i < blen >> 1; ++i)
		    *fp++ = static_cast<float>(src[i]) / 0x8000;
	    }
	    break;
	case 24:
	    {
		if (sf.m_endian == SampleFormat::kIsBigEndian)
		    bswap24buffer(bp, blen);
		for (size_t i = 0; i < blen / 3; ++i) {
		    int32_t v = bp[i*3]<<8 | bp[i*3+1]<<16 | bp[i*3+2]<<24;
		    *fp++ = static_cast<float>(v) / 0x80000000U;
		}
	    }
	    break;
	case 32:
	    {
		if (sf.m_endian == SampleFormat::kIsBigEndian)
		    bswap32buffer(bp, blen);
		int *src = reinterpret_cast<int *>(bp);
		for (size_t i = 0; i < blen >> 2; ++i)
		    *fp++ = static_cast<float>(src[i]) / 0x80000000U;
	    }
	    break;
	}
    }
    return nsamples;
}

