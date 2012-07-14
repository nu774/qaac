#include <cstdio>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#if defined(_MSC_VER ) || defined(__MINGW32__)
#include <io.h>
#else
#include <unistd.h>
#endif
#include "wavsource.h"
#include "wavsink.h"

namespace wave {
    template <typename T>
    void put(std::streambuf *os, T obj)
    {
	os->sputn(reinterpret_cast<char*>(&obj), sizeof obj);
    }

    void buildHeader(const SampleFormat &format, uint32_t chanmask,
		     std::streambuf *result)
    {
	uint16_t fmt;
	// wFormatTag
	{
	    fmt = ((chanmask && format.m_nchannels > 2)
		   || format.m_bitsPerSample > 16)
		    ? kFormatExtensible : kFormatPCM;
	    put(result, fmt);
	}
	// nChannels
	put(result, static_cast<uint16_t>(format.m_nchannels));
	// nSamplesPerSec
	put(result, static_cast<uint32_t>(format.m_rate));

	uint16_t bpf = format.bytesPerFrame();
	// nAvgBytesPerSec
	put(result, static_cast<uint32_t>(format.m_rate * bpf));
	// nBlockAlign
	put(result, bpf);
	// wBitsPerSample
	put(result, static_cast<uint16_t>(format.m_bitsPerSample));

	// cbSize
	if (fmt != kFormatPCM) {
	    // WAVEFORMATEXTENSIBLE
	    put(result, static_cast<uint16_t>(22));
	    // Samples
	    put(result, static_cast<uint16_t>(format.m_bitsPerSample));
	    // dwChannelMask
	    put(result, chanmask);
	    // SubFormat
	    if (format.m_type == SampleFormat::kIsFloat)
		put(result, ksFormatSubTypeFloat);
	    else
		put(result, ksFormatSubTypePCM);
	}
    }
}

WaveSink::WaveSink(FILE *fp,
		   uint64_t duration,
		   const SampleFormat &format,
		   uint32_t chanmask)
	: m_file(fp), m_bytes_written(0), m_closed(false),
	  m_seekable(false), m_format(format)
{
    struct stat stb = { 0 };
    if (fstat(fileno(fp), &stb))
	throw_crt_error("fstat()");
    m_seekable = ((stb.st_mode & S_IFMT) == S_IFREG);
    std::ostringstream os;
    wave::buildHeader(format, chanmask, os.rdbuf());
    std::string header = os.str();

    uint32_t hdrsize = header.size();
    uint32_t riffsize = 0, datasize = 0;
    if (duration != -1) {
	uint64_t datasize64 = duration * format.bytesPerFrame();
	uint64_t riffsize64 = hdrsize + datasize64 + 20;
	if (riffsize64 >> 32 == 0) {
	    datasize = static_cast<uint32_t>(datasize64);
	    riffsize = static_cast<uint32_t>(riffsize64);
	}
    }
    write("RIFF", 4);
    write(&riffsize, 4);
    write("WAVEfmt ", 8);
    write(&hdrsize, 4);
    write(header.c_str(), hdrsize);
    write("data", 4);
    write(&datasize, 4);
    m_data_pos = 28 + hdrsize;
    if (!m_seekable) std::fflush(fp);
}

void WaveSink::writeSamples(const void *data, size_t length, size_t nsamples)
{
    uint8_t *bp = static_cast<uint8_t *>(const_cast<void*>(data));
    std::vector<uint8_t> buf;
    if (m_format.m_endian == SampleFormat::kIsBigEndian) {
	buf.resize(length);
	bp = &buf[0];
	std::memcpy(bp, data, length);
	switch (m_format.m_bitsPerSample) {
	case 16: bswap16buffer(bp, length); break;
	case 24: bswap24buffer(bp, length); break;
	case 32: bswap32buffer(bp, length); break;
	case 64: bswap64buffer(bp, length); break;
	}
    }
    if (m_format.m_bitsPerSample == 8 &&
	m_format.m_type == SampleFormat::kIsSignedInteger) {
	if (!buf.size() != length) {
	    buf.resize(length);
	    bp = &buf[0];
	    std::memcpy(bp, data, length);
	}
	for (size_t i = 0; i < length; ++i)
	    bp[i] ^= 0x80;
    }
    write(bp, length);
    if (!m_seekable) std::fflush(m_file);
    m_bytes_written += length;
}

void WaveSink::finishWrite()
{
    if (m_closed) return;
    m_closed = true;
    if (m_bytes_written & 1) write("\0", 1);
    if (!m_seekable) return;
    uint64_t datasize64 = m_bytes_written;
    uint64_t riffsize64 = datasize64 + m_data_pos - 8;
    fpos_t pos = 0;
    if (riffsize64 >> 32 == 0 && std::fgetpos(m_file, &pos) == 0) {
	if (std::fseek(m_file, m_data_pos - 4, SEEK_SET) == 0) {
	    uint32_t size32 = static_cast<uint32_t>(datasize64);
	    write(&size32, 4);
	    if (std::fseek(m_file, 4, SEEK_SET) == 0) {
		size32 = static_cast<uint32_t>(riffsize64);
		write(&size32, 4);
	    }
	}
    }
}


