#include <cstdio>
#include <iostream>
#include <sstream>
#include "wavsource.h"
#include "wavsink.h"

namespace wav {
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

WavSink::WavSink(FILE *fp,
		 uint64_t duration,
		 const SampleFormat &format,
		 uint32_t chanmask)
	: m_file(fp)
{
    std::ostringstream os;
    wav::buildHeader(format, chanmask, os.rdbuf());
    std::string header = os.str();

    uint32_t hdrsize = header.size();
    uint64_t datasize_ = duration * format.bytesPerFrame();
    uint64_t riffsize_ = hdrsize + datasize_ + 20; 
    uint32_t datasize = static_cast<uint32_t>(datasize_); // XXX
    uint32_t riffsize = static_cast<uint32_t>(riffsize_); // XXX

    write("RIFF", 4);
    write(&riffsize, 4);
    write("WAVEfmt ", 8);
    write(&hdrsize, 4);
    write(header.c_str(), hdrsize);
    write("data", 4);
    write(&datasize, 4);
}
