#include <cstdio>
#include <cstring>
#include "normalize.h"
#include "win32util.h"

Normalizer::Normalizer(const x::shared_ptr<ISource> &src)
    : DelegatingSource(src), m_peak(0.0), m_processed(0)
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
    if (srcFormat.m_bitsPerSample == 64)
	throw std::runtime_error("Can't handle 64bit sample");

    m_format = SampleFormat("F32LE", srcFormat.m_nchannels, srcFormat.m_rate);
    m_ibuffer.resize(4096 * srcFormat.bytesPerFrame());
    m_fbuffer.resize(4096 * m_format.m_nchannels);

    FILE *tmpfile = win32_tmpfile(L"qaac.norm");
    m_tmpfile = x::shared_ptr<FILE>(tmpfile, std::fclose);
}

size_t Normalizer::process(size_t nsamples)
{
    size_t nc = underflow(&m_fbuffer[0], nsamples);
    if (nc > 0) {
	std::fwrite(&m_fbuffer[0], sizeof(float), nc * m_format.m_nchannels,
		m_tmpfile.get());
	if (std::ferror(m_tmpfile.get()))
	    throw std::runtime_error(format("fwrite: %s",
			std::strerror(errno)));
	for (size_t i = 0; i < m_fbuffer.size(); ++i) {
	    float x = std::fabs(m_fbuffer[i]);
	    if (x > m_peak) m_peak = x;
	}
    } else
	std::fseek(m_tmpfile.get(), 0, SEEK_SET);
    return nc;
}

size_t Normalizer::readSamples(void *buffer, size_t nsamples)
{
    size_t nc = std::fread(buffer, sizeof(float),
 	    nsamples * m_format.m_nchannels, m_tmpfile.get());
    float *fp = reinterpret_cast<float*>(buffer);
    if (m_peak > 1.0 || (m_peak > FLT_EPSILON && m_peak < 1.0 - FLT_EPSILON)) {
	for (size_t i = 0; i < nc; ++i) {
	    float nfp = static_cast<float>(*fp / m_peak);
	    *fp++ = nfp;
	}
    }
    return nc / m_format.m_nchannels;
}

size_t Normalizer::underflow(float *buffer, size_t nsamples)
{
    const SampleFormat &srcFormat = source()->getSampleFormat();
    size_t bpf = srcFormat.bytesPerFrame();
    size_t blen = nsamples * bpf;
    size_t nread;

    if (srcFormat.m_type == SampleFormat::kIsFloat) {
	nread = source()->readSamples(&buffer[0], nsamples);
	if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
	    bswap32buffer(reinterpret_cast<uint8_t*>(&buffer[0]), blen);
	m_processed += nread;
	return nread;
    }
    if (m_ibuffer.size() < blen)
	m_ibuffer.resize(blen);
    nread = source()->readSamples(&m_ibuffer[0], nsamples);
    m_processed += nread;
    blen = nread * bpf;
    float *fp = buffer;

    switch (srcFormat.m_bitsPerSample) {
    case 8:
	{
	    if (srcFormat.m_type == SampleFormat::kIsUnsignedInteger) {
		uint8_t *src = &m_ibuffer[0];
		for (size_t i = 0; i < blen; ++i)
		    src[i] = src[i] ^ 0x80;
	    }
	    const char *src = reinterpret_cast<const char *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x80;
	}
	break;
    case 16:
	{
	    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
		bswap16buffer(&m_ibuffer[0], blen);
	    const short *src = reinterpret_cast<const short *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen / 2; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x8000;
	}
	break;
    case 24:
	{
	    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
		bswap24buffer(&m_ibuffer[0], blen);
	    const uint8_t *src =
		reinterpret_cast<const uint8_t*>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen / 3; ++i) {
		int32_t hv = static_cast<int8_t>(src[i*3+2]);
		int32_t v = ((src[i*3] << 8)| (src[i*3+1] << 16) | (hv << 24));
		*fp++ = static_cast<float>(v) / 0x80000000U;
	    }
	}
	break;
    case 32:
	{
	    if (srcFormat.m_endian == SampleFormat::kIsBigEndian)
		bswap32buffer(&m_ibuffer[0], blen);
	    const int *src = reinterpret_cast<const int *>(&m_ibuffer[0]);
	    for (size_t i = 0; i < blen / 4; ++i)
		*fp++ = static_cast<float>(src[i]) / 0x80000000U;
	}
	break;
    }
    return nread;
}
