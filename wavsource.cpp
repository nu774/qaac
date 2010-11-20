#include "wavsource.h"
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "itunetags.h"

struct myGUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];

    bool operator==(const myGUID &rhs)
    {
	return !std::memcmp(this, &rhs, sizeof(myGUID));
    }
};

namespace __wave {
    enum {
	kFormatPCM = 1,
	kFormatFloat = 3,
	kFormatExtensible = 0xfffe
    };
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
    uint32_t itunes_tagid(uint32_t fcc)
    {
	switch (fcc) {
	    case 'INAM': return Tag::kTitle;
	    case 'IART': return Tag::kArtist;
	    case 'IPRD': return Tag::kAlbum;
	    case 'ICMT': return Tag::kComment;
	    case 'IGNR': return Tag::kGenre;
	    case 'ICRD': return Tag::kDate;
	    case 'ITRK': return Tag::kTrack;
	    case 'ICOP': return Tag::kCopyright;
	}
	return 0;
    }
}

WaveSource::WaveSource(InputStream &stream, bool ignorelength)
    : RIFFParser(stream), m_samples_read(0), m_ignore_length(ignorelength)
{
    parse();
    if (format_id() != 'WAVE')
	throw std::runtime_error("Not a WAV file");
    while (next() && chunk_id() != 'data') {
	if (chunk_id() == 'fmt ')
	    fetchWaveFormat();
    }
    if (!m_format.m_bitsPerSample)
	__wave::want(false);

    m_samples_read = 0;
    if (ignorelength || chunk_size() == 0xffffffff)
	/* streaming? */
	m_duration = -1;
    else
	m_duration = chunk_size() / m_format.bytesPerFrame();
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
    __wave::want(wformat == __wave::kFormatPCM
	      || wformat == __wave::kFormatFloat
	      || wformat == __wave::kFormatExtensible);
    if (wformat == __wave::kFormatFloat)
	m_format.m_type = SampleFormat::kIsFloat;
    // nChannels
    check_eof(read16le(&nchannels));
    __wave::want(nchannels > 0 && nchannels < 9);
    m_format.m_nchannels = nchannels;
    // nSamplesPerSec
    check_eof(read32le(&y));
    __wave::want(y > 0);
    m_format.m_rate = y;
    // nAvgBytesPerSec
    check_eof(read32le(&y));
    // nBlockAlign
    check_eof(read16le(&x));
    // wBitsPerSample
    check_eof(read16le(&x));
    __wave::want(x > 0 && (x & 0x7) == 0);
    m_format.m_bitsPerSample = x;
    if (wformat == __wave::kFormatPCM)
	m_format.m_type = x > 8 ? SampleFormat::kIsSignedInteger
	    			: SampleFormat::kIsUnsignedInteger;

    if (wformat == __wave::kFormatExtensible) {
	if (chunk_size() < 40)
	    throw std::runtime_error("Invalid fmt chunk");
	// cbSize
	check_eof(read16le(&x));
	// wValidBitsPerSample
	check_eof(read16le(&x));
	__wave::want(x == m_format.m_bitsPerSample);
	// dwChannelMask
	check_eof(read32le(&y));
	__wave::want(bitcount(y) == nchannels);
	for (size_t i = 0; i < 32; ++i, y >>= 1)
	    if (y & 1) m_chanmap.push_back(i + 1);

	// SubFormat
	myGUID subFormat;
	check_eof(read32le(&subFormat.Data1));
	check_eof(read16le(&subFormat.Data2));
	check_eof(read16le(&subFormat.Data3));
	check_eof(read(&subFormat.Data4, 8) == 8);
	if (subFormat == __wave::ksFormatSubTypePCM)
	    m_format.m_type = x > 8 ? SampleFormat::kIsSignedInteger
				    : SampleFormat::kIsUnsignedInteger;
	else if (subFormat == __wave::ksFormatSubTypeFloat)
	    m_format.m_type = SampleFormat::kIsFloat;
	else
	    __wave::want(false);
    }
}

void WaveSource::setRange(int64_t start, int64_t length)
{
    int64_t dur = static_cast<int64_t>(m_duration);
    if (length >= 0 && (dur == -1 || length < dur))
	m_duration = length;
    if (start > 0 && seek_forwardx(start) != start)
	throw std::runtime_error("seek_forward failed");
    if (start > 0 && dur > 0 && length == -1)
	m_duration -= start;
}

