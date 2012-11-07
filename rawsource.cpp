#include "rawsource.h"
#include "win32util.h"
#include "cautil.h"

RawSource::RawSource(const std::shared_ptr<FILE> &fp,
		     const AudioStreamBasicDescription &asbd)
    : m_fp(fp), m_asbd(asbd)
{
    int64_t len = _filelengthi64(fileno(m_fp.get()));
    setRange(0, len == -1 ? -1 : len / asbd.mBytesPerFrame);
    bool isfloat = asbd.mFormatFlags & kAudioFormatFlagIsFloat;
    m_oasbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
				       asbd.mChannelsPerFrame,
				       asbd.mBitsPerChannel,
				       isfloat ? asbd.mBitsPerChannel : 32,
				       isfloat ? kAudioFormatFlagIsFloat
				          : kAudioFormatFlagIsSignedInteger);
}

size_t RawSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (nsamples) {
	ssize_t nbytes = nsamples * m_asbd.mBytesPerFrame;
	m_buffer.resize(nbytes);
	nbytes = read(fileno(m_fp.get()), &m_buffer[0], nbytes);
	nsamples = nbytes > 0 ? nbytes / m_asbd.mBytesPerFrame : 0;
	if (nsamples) {
	    size_t size = nsamples * m_asbd.mBytesPerFrame;
	    uint8_t *bp = &m_buffer[0];
	    /* bswap */
	    if (m_asbd.mFormatFlags & kAudioFormatFlagIsBigEndian)
		util::bswapbuffer(bp, size, (m_asbd.mBitsPerChannel + 7) &~7);
	    /* convert to signed */
	    if (!(m_asbd.mFormatFlags & kAudioFormatFlagIsFloat) &&
		!(m_asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger))
	    {
		size_t count = nsamples * m_asbd.mChannelsPerFrame;
		uint32_t width = ((m_asbd.mBitsPerChannel + 7) & ~7) / 8;
		for (size_t i = 1; i <= count; ++i)
		    bp[i * width - 1] ^= 0x80;

	    }
	    util::unpack(bp, buffer, &size,
			 m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame,
			 m_oasbd.mBytesPerFrame / m_oasbd.mChannelsPerFrame);
	}
	addSamplesRead(nsamples);
    }
    return nsamples;
}

void RawSource::skipSamples(int64_t count)
{
    int64_t bytes = count * m_asbd.mBytesPerFrame;
    int fd = fileno(m_fp.get());
    if (util::is_seekable(fd))
	_lseeki64(fd, bytes, SEEK_CUR);
    else {
	char buf[0x1000];
	while (bytes > 0) {
	    int n = std::min(bytes, 0x1000LL);
	    int nn = read(fd, buf, n);
	    if (nn <= 0) break;
	    bytes -= nn;
	}
    }
}
