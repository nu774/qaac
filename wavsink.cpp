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

    void buildHeader(const SampleFormat &format,
	    const std::vector<uint32_t> *chanmap, std::streambuf *result)
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
	    put(result, static_cast<uint16_t>(0));
	    // dwChannelMask
	    {
		uint32_t mask = 0;
		if (chanmap)
		    for (size_t i = 0; i < chanmap->size(); ++i)
			mask |= (1 << (chanmap->at(i) - 1));
		else {
		    static uint32_t default_map[] = {
			1, 2, 7, 0x107, 0x37, 0x3f, 0x70f, 0x63f
		    };
		    mask = default_map[format.m_nchannels - 1];
		}
		put(result, mask);
	    }
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
		 const std::vector<uint32_t> *chanmap)
	: m_file(fp)
{
    std::ostringstream os;
    wav::buildHeader(format, chanmap, os.rdbuf());
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
