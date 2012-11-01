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
    if (!m_asbd.mBitsPerChannel)
	wave::want(false);

    if (ignorelength || !chunk_size() ||
	chunk_size() % m_asbd.mBytesPerFrame) {
	setRange(0, -1);
	m_ignore_length = true;
    } else
	setRange(0, chunk_size() / m_asbd.mBytesPerFrame);
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
    util::check_eof(read16le(&wformat));
    wave::want(wformat == wave::kFormatPCM
	      || wformat == wave::kFormatFloat
	      || wformat == wave::kFormatExtensible);
    if (wformat == wave::kFormatFloat)
	type = kAudioFormatFlagIsFloat;
    // nChannels
    util::check_eof(read16le(&nChannels));
    wave::want(nChannels > 0 && nChannels < 9);
    // nSamplesPerSec
    util::check_eof(read32le(&nSamplesPerSec));
    wave::want(nSamplesPerSec > 0);
    // nAvgBytesPerSec
    util::check_eof(read32le(&y));
    // nBlockAlign
    util::check_eof(read16le(&nBlockAlign));
    // wBitsPerSample
    util::check_eof(read16le(&wBitsPerSample));
    wave::want(wBitsPerSample > 0 && (wBitsPerSample & 0x7) == 0);
    wValidBitsPerSample = wBitsPerSample;
    if (wformat == wave::kFormatPCM)
	type = wBitsPerSample > 8
	       ? kAudioFormatFlagIsSignedInteger : 0;

    if (wformat == wave::kFormatExtensible) {
	if (chunk_size() < 40)
	    throw std::runtime_error("Invalid fmt chunk");
	// cbSize
	util::check_eof(read16le(&x));
	// wValidBitsPerSample
	util::check_eof(read16le(&wValidBitsPerSample));
	wave::want(wValidBitsPerSample > 0 &&
		   wValidBitsPerSample <= wBitsPerSample);
	// dwChannelMask
	util::check_eof(read32le(&dwChannelMask));
	if (dwChannelMask > 0 && util::bitcount(dwChannelMask) >= nChannels)
	    chanmap::getChannels(dwChannelMask, &m_chanmap, nChannels);
	// SubFormat
	wave::myGUID subFormat;
	util::check_eof(read32le(&subFormat.Data1));
	util::check_eof(read16le(&subFormat.Data2));
	util::check_eof(read16le(&subFormat.Data3));
	util::check_eof(read(&subFormat.Data4, 8) == 8);
	if (subFormat == wave::ksFormatSubTypePCM)
	    type = x > 8 ? kAudioFormatFlagIsSignedInteger : 0;
	else if (subFormat == wave::ksFormatSubTypeFloat)
	    type = kAudioFormatFlagIsFloat;
	else
	    wave::want(false);
    }
    std::memset(&m_asbd, 0, sizeof m_asbd);
    m_asbd.mFormatID = 'lpcm';
    m_asbd.mFormatFlags = type;
    m_asbd.mFormatFlags |=
	(wValidBitsPerSample & 7) ? kAudioFormatFlagIsAlignedHigh
				  : kAudioFormatFlagIsPacked;
    m_asbd.mSampleRate = nSamplesPerSec;
    m_asbd.mChannelsPerFrame = nChannels;
    m_asbd.mBitsPerChannel = wValidBitsPerSample;
    m_asbd.mFramesPerPacket = 1;
    m_asbd.mBytesPerFrame = nBlockAlign;
    m_asbd.mBytesPerPacket =
	m_asbd.mFramesPerPacket * m_asbd.mBytesPerFrame;
}
