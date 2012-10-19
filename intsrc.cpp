#include "intsrc.h"

static inline double clip(double x, double max, double min)
{
    if (x > max) x = max;
    else if (x < min) x = min;
    return x;
}

size_t IntegerSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = readSamplesAsFloat(source(), &m_ibuffer, &m_fbuffer, nsamples);
    if (!nsamples)
	return 0;
    float *fp = &m_fbuffer[0];
    size_t count = nsamples * m_format.m_nchannels;
    switch (m_format.bytesPerChannel()) {
    case 2:
	{
	    int16_t *dst = static_cast<int16_t*>(buffer);
	    for (size_t i = 0; i < count; ++i) {
		double v = fp[i] * 0x8000;
		// dither with TPDF
		double rv = v + random() + random();
		*dst++ = lrint(clip(rv, 32767.0, -32768.0));
	    }
	}
	break;
    case 3:
	{
	    uint8_t *dst = static_cast<uint8_t*>(buffer);
	    for (size_t i = 0; i < count; ++i) {
		double v = fp[i] * 0x800000;
		int32_t iv = lrint(clip(v, 8388607.0, -8388608.0));
		*dst++ = iv;
		*dst++ = iv >> 8;
		*dst++ = iv >> 16;
	    }
	}
	break;
    case 4:
	{
	    int32_t *dst = static_cast<int32_t*>(buffer);
	    for (size_t i = 0; i < count; ++i) {
		double v = fp[i] * 0x80000000u;
		*dst++ = lrint(clip(v, 2147483647.0, -2147483648.0));
	    }
	}
	break;
    }
    return nsamples;
}
