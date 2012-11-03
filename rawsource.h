#ifndef _RAWSOURCE_H
#define _RAWSOURCE_H

#include "iointer.h"
#include "win32util.h"

class RawSource: public PartialSource<RawSource> {
    std::shared_ptr<FILE> m_fp;
    AudioStreamBasicDescription m_asbd;
public:
    RawSource(const std::wstring &path, const AudioStreamBasicDescription &asbd)
	: m_asbd(asbd)
    {
	m_fp = win32::fopen(path, L"rb");
	int64_t len = _filelengthi64(fileno(m_fp.get()));
	setRange(0, len == -1 ? -1 : len / asbd.mBytesPerFrame);
    }
    uint64_t length() const { return getDuration(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	nsamples = adjustSamplesToRead(nsamples);
	if (nsamples) {
	    size_t nblocks = m_asbd.mBytesPerFrame;
	    nsamples =
		read(fileno(m_fp.get()), buffer, nsamples * nblocks) / nblocks;
	    addSamplesRead(nsamples);
	}
	return nsamples;
    }
    void skipSamples(int64_t count)
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
};

#endif
