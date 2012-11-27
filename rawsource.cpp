#include "rawsource.h"
#include "win32util.h"
#include "cautil.h"

RawSource::RawSource(const std::shared_ptr<FILE> &fp,
		     const AudioStreamBasicDescription &asbd)
    : m_position(0), m_fp(fp), m_asbd(asbd)
{
    int64_t len = _filelengthi64(fileno(m_fp.get()));
    m_length = (len == -1 ? -1 : len / asbd.mBytesPerFrame);
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
    ssize_t nbytes = nsamples * m_asbd.mBytesPerFrame;
    if (m_buffer.size() < nbytes)
	m_buffer.resize(nbytes);
    nbytes = util::nread(fileno(m_fp.get()), &m_buffer[0], nbytes);
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
    m_position += nsamples;
    return nsamples;
}

void RawSource::seekTo(int64_t count)
{
    int fd = fileno(m_fp.get());
    if (util::is_seekable(fd)) {
	CHECKCRT(_lseeki64(fd, count*m_asbd.mBytesPerFrame, SEEK_SET) < 0);
	m_position = count;
    } else if (m_position > count) {
	throw std::runtime_error("Cannot seek back the input");
    } else {
	int64_t bytes = (count - m_position) * m_asbd.mBytesPerFrame;
	int64_t nread = 0;
	char buf[0x1000];
	while (nread < bytes) {
	    int n = util::nread(fd, buf, std::min(bytes - nread, 0x1000LL));
	    if (n <= 0) break;
	    nread += n;
	}
	m_position += nread / m_asbd.mBytesPerFrame;
    }
}
