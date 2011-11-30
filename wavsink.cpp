#include <cstdio>
#include <iostream>
#include <sstream>
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
	if (format.m_nchannels > 8)
	    throw std::runtime_error(
		    "Sorry, can't handle more than 8 channels");
	if (format.m_bitsPerSample == 8 &&
		format.m_type == SampleFormat::kIsSignedInteger)
	    throw std::runtime_error("Sorry, can't handle 8bit signed LPCM");
	if (format.m_endian == SampleFormat::kIsBigEndian)
	    throw std::runtime_error("Sorry, can't handle big endian LPCM");

	// wFormatTag
	{
	    uint16_t fmt;
	    if (format.m_nchannels > 2)
		fmt = kFormatExtensible;
	    else if (format.m_type == SampleFormat::kIsFloat)
		fmt = kFormatFloat;
	    else
		fmt = kFormatPCM;
	    put(result, static_cast<uint16_t>(fmt));
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
	if (format.m_nchannels <= 2)
	    put(result, static_cast<uint16_t>(0));
	else {
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
	: m_file(fp), m_bytes_written(0)
{
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
}

void WaveSink::finishWrite()
{
    if (m_bytes_written & 1) write("\0", 1);
    uint64_t datasize64 = m_bytes_written;
    uint64_t riffsize64 = datasize64 + m_data_pos - 8;
    fpos_t pos;
    if (riffsize64 >> 32 == 0 && std::fgetpos(m_file, &pos) == 0) {
	if (std::fseek(m_file, m_data_pos - 4, SEEK_SET) == 0) {
	    uint32_t size32 = static_cast<uint32_t>(datasize64);
	    write(&size32, 4);
	    if (std::fseek(m_file, 4, SEEK_SET) == 0) {
		size32 = static_cast<uint32_t>(riffsize64);
		write(&size32, 4);
	    }
	    std::fsetpos(m_file, &pos);
	}
    }
}
