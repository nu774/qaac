#include "wavsource.h"

namespace wave {
    inline void want(bool expr)
    {
	if (!expr)
	    throw std::runtime_error("Sorry, unacceptable WAVE format");
    }
    myGUID ksFormatSubTypePCM = {
	0x1, 0x0, 0x10, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
    };
    myGUID ksFormatSubTypeFloat = {
	0x3, 0x0, 0x10, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
    };
}

WaveSource::WaveSource(InputStream &stream, bool ignorelength)
    : RIFFParser(stream), m_ignore_length(ignorelength)
{
    parse();
    if (format_id() != 'WAVE')
	throw std::runtime_error("Not a WAV file");
    while (next() && chunk_id() != 'data') {
	if (chunk_id() == 'fmt ')
	    fetchWaveFormat();
    }
    if (!m_format.m_bitsPerSample)
	wave::want(false);

    if (ignorelength || chunk_size() % m_format.bytesPerFrame())
	setRange(0, -1);
    else
	setRange(0, chunk_size() / m_format.bytesPerFrame());
}

void WaveSource::fetchWaveFormat()
{
    if (chunk_size() < 16)
	throw std::runtime_error("Invalid fmt chunk");

    m_format.m_endian = SampleFormat::kIsLittleEndian;

    uint16_t wformat, x, nchannels;
    uint32_t y;
    // wFormatTag
    check_eof(read16le(&wformat));
    wave::want(wformat == wave::kFormatPCM
	      || wformat == wave::kFormatFloat
	      || wformat == wave::kFormatExtensible);
    if (wformat == wave::kFormatFloat)
	m_format.m_type = SampleFormat::kIsFloat;
    // nChannels
    check_eof(read16le(&nchannels));
    wave::want(nchannels > 0 && nchannels < 9);
    m_format.m_nchannels = nchannels;
    // nSamplesPerSec
    check_eof(read32le(&y));
    wave::want(y > 0);
    m_format.m_rate = y;
    // nAvgBytesPerSec
    check_eof(read32le(&y));
    // nBlockAlign
    check_eof(read16le(&x));
    // wBitsPerSample
    check_eof(read16le(&x));
    wave::want(x > 0 && (x & 0x7) == 0);
    m_format.m_bitsPerSample = x;
    if (wformat == wave::kFormatPCM)
	m_format.m_type = x > 8 ? SampleFormat::kIsSignedInteger
	    			: SampleFormat::kIsUnsignedInteger;

    if (wformat == wave::kFormatExtensible) {
	if (chunk_size() < 40)
	    throw std::runtime_error("Invalid fmt chunk");
	// cbSize
	check_eof(read16le(&x));
	// wValidBitsPerSample
	check_eof(read16le(&x));
	wave::want(x == m_format.m_bitsPerSample);
	// dwChannelMask
	check_eof(read32le(&y));
	if (y > 0) {
	    wave::want(bitcount(y) == nchannels);
	    for (size_t i = 0; i < 32; ++i, y >>= 1)
		if (y & 1) m_chanmap.push_back(i + 1);
	}
	// SubFormat
	wave::myGUID subFormat;
	check_eof(read32le(&subFormat.Data1));
	check_eof(read16le(&subFormat.Data2));
	check_eof(read16le(&subFormat.Data3));
	check_eof(read(&subFormat.Data4, 8) == 8);
	if (subFormat == wave::ksFormatSubTypePCM)
	    m_format.m_type = x > 8 ? SampleFormat::kIsSignedInteger
				    : SampleFormat::kIsUnsignedInteger;
	else if (subFormat == wave::ksFormatSubTypeFloat)
	    m_format.m_type = SampleFormat::kIsFloat;
	else
	    wave::want(false);
    }
}
