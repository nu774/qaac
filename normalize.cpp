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
    const AudioStreamBasicDescription &srcFormat = source()->getSampleFormat();
    if (srcFormat.mBitsPerChannel == 64)
	throw std::runtime_error("Can't handle 64bit sample");

    m_asbd = cautil::buildASBDForPCM(srcFormat.mSampleRate,
				     srcFormat.mChannelsPerFrame, 32,
				     kAudioFormatFlagIsFloat);

    if (!seekable) {
	FILE *tmpfile = win32::tmpfile(L"qaac.norm");
	m_tmpfile = std::shared_ptr<FILE>(tmpfile, std::fclose);
    }
}

size_t Normalizer::process(size_t nsamples)
{
    size_t nc = readSamplesAsFloat(source(), &m_ibuffer, &m_fbuffer, nsamples);
    if (nc > 0) {
	m_processed += nc;
	if (m_tmpfile.get()) {
	    std::fwrite(&m_fbuffer[0], sizeof(float),
			nc * m_asbd.mChannelsPerFrame, m_tmpfile.get());
	    if (std::ferror(m_tmpfile.get()))
		util::throw_crt_error("fwrite()");
	}
	for (size_t i = 0; i < m_fbuffer.size(); ++i) {
	    float x = std::abs(m_fbuffer[i]);
	    if (x > m_peak) m_peak = x;
	}
    } else if (m_tmpfile.get())
	std::fseek(m_tmpfile.get(), 0, SEEK_SET);
    return nc;
}

size_t Normalizer::readSamples(void *buffer, size_t nsamples)
{
    if (!m_tmpfile.get())
	return 0;
    size_t nc = std::fread(buffer, sizeof(float),
			   nsamples * m_asbd.mChannelsPerFrame,
			   m_tmpfile.get());
    float *fp = reinterpret_cast<float*>(buffer);
    if (m_peak > FLT_MIN) {
	for (size_t i = 0; i < nc; ++i) {
	    float nfp = static_cast<float>(*fp / m_peak);
	    *fp++ = nfp;
	}
    }
    nsamples = nc / m_asbd.mChannelsPerFrame;
    m_position += nsamples;
    return nsamples;
}
