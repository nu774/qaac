#include "wavsource.h"
#include "chanmap.h"

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
    if (!m_format.mBitsPerChannel)
	wave::want(false);

    if (ignorelength || !chunk_size() ||
	chunk_size() % m_format.mBytesPerFrame) {
	setRange(0, -1);
	m_ignore_length = true;
    } else
	setRange(0, chunk_size() / m_format.mBytesPerFrame);
}

void WaveSource::fetchWaveFormat()
{
    if (chunk_size() < 16)
	throw std::runtime_error("Invalid fmt chunk");

    uint16_t wformat, x;
    uint16_t nChannels, nBlockAlign, wBitsPerSample, wValidBitsPerSample;
    uint32_t nSamplesPerSec, dwChannelMask = 0;
    uint32_t y;
    uint32_t type;
    // wFormatTag
    check_eof(read16le(&wformat));
    wave::want(wformat == wave::kFormatPCM
	      || wformat == wave::kFormatFloat
	      || wformat == wave::kFormatExtensible);
    if (wformat == wave::kFormatFloat)
	type = kAudioFormatFlagIsFloat;
    // nChannels
    check_eof(read16le(&nChannels));
    wave::want(nChannels > 0 && nChannels < 9);
    // nSamplesPerSec
    check_eof(read32le(&nSamplesPerSec));
    wave::want(nSamplesPerSec > 0);
    // nAvgBytesPerSec
    check_eof(read32le(&y));
    // nBlockAlign
    check_eof(read16le(&nBlockAlign));
    // wBitsPerSample
    check_eof(read16le(&wBitsPerSample));
    wave::want(wBitsPerSample > 0 && (wBitsPerSample & 0x7) == 0);
    wValidBitsPerSample = wBitsPerSample;
    if (wformat == wave::kFormatPCM)
	type = wBitsPerSample > 8
	       ? kAudioFormatFlagIsSignedInteger : 0;

    if (wformat == wave::kFormatExtensible) {
	if (chunk_size() < 40)
	    throw std::runtime_error("Invalid fmt chunk");
	// cbSize
	check_eof(read16le(&x));
	// wValidBitsPerSample
	check_eof(read16le(&wValidBitsPerSample));
	wave::want(wValidBitsPerSample > 0 &&
		   wValidBitsPerSample <= wBitsPerSample);
	// dwChannelMask
	check_eof(read32le(&dwChannelMask));
	if (dwChannelMask > 0 && bitcount(dwChannelMask) >= nChannels)
	    chanmap::GetChannels(dwChannelMask, &m_chanmap, nChannels);
	// SubFormat
	wave::myGUID subFormat;
	check_eof(read32le(&subFormat.Data1));
	check_eof(read16le(&subFormat.Data2));
	check_eof(read16le(&subFormat.Data3));
	check_eof(read(&subFormat.Data4, 8) == 8);
	if (subFormat == wave::ksFormatSubTypePCM)
	    type = x > 8 ? kAudioFormatFlagIsSignedInteger : 0;
	else if (subFormat == wave::ksFormatSubTypeFloat)
	    type = kAudioFormatFlagIsFloat;
	else
	    wave::want(false);
    }
    std::memset(&m_format, 0, sizeof m_format);
    m_format.mFormatID = 'lpcm';
    m_format.mFormatFlags = type;
    m_format.mFormatFlags |=
	(wValidBitsPerSample & 7) ? kAudioFormatFlagIsAlignedHigh
				  : kAudioFormatFlagIsPacked;
    m_format.mSampleRate = nSamplesPerSec;
    m_format.mChannelsPerFrame = nChannels;
    m_format.mBitsPerChannel = wValidBitsPerSample;
    m_format.mFramesPerPacket = 1;
    m_format.mBytesPerFrame = nBlockAlign;
    m_format.mBytesPerPacket =
	m_format.mFramesPerPacket * m_format.mBytesPerFrame;
}
