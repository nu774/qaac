#ifndef ENDIANCONVERTER_H
#define ENDIANCONVERTER_H

#include "iointer.h"

class EndianConverter: public DelegatingSource {
    AudioStreamBasicDescription m_asbd;
public:
    EndianConverter(const std::shared_ptr<ISource> &source)
	: DelegatingSource(source)
    {
	m_asbd = source->getSampleFormat();
	if (m_asbd.mFormatFlags & kAudioFormatFlagIsBigEndian)
	    m_asbd.mFormatFlags &= ~kAudioFormatFlagIsBigEndian;
	else
	    m_asbd.mFormatFlags |= kAudioFormatFlagIsBigEndian;
    }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	size_t n = source()->readSamples(buffer, nsamples);
	uint8_t *bp = static_cast<uint8_t*>(buffer);
	switch (m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame) {
	case 2:
	    util::bswap16buffer(bp, n * m_asbd.mBytesPerFrame);
	    break;
	case 3:
	    util::bswap24buffer(bp, n * m_asbd.mBytesPerFrame);
	    break;
	case 4:
	    util::bswap32buffer(bp, n * m_asbd.mBytesPerFrame);
	    break;
	case 8:
	    util::bswap64buffer(bp, n * m_asbd.mBytesPerFrame);
	    break;
	}
	return n;
    }
};

#endif
