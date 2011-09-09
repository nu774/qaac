#ifndef _WAVSOURCE_H
#define _WAVSOURCE_H

#include <algorithm>
#include "iointer.h"
#include "riff.h"

namespace wav
{
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
    enum {
	kFormatPCM = 1,
	kFormatFloat = 3,
	kFormatExtensible = 0xfffe
    };
    extern myGUID ksFormatSubTypePCM, ksFormatSubTypeFloat;
}

class WaveSource :
    public ISource, private RIFFParser, public PartialSource<WaveSource>
{
    SampleFormat m_format;
    std::vector<uint32_t> m_chanmap;
    bool m_ignore_length;
public:
    explicit WaveSource(InputStream &stream, bool ignorelength=false);
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const
    {
	return m_chanmap.size() ? &m_chanmap : 0;
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	nsamples = adjustSamplesToRead(nsamples);
	if (nsamples) {
	    size_t nblocks = m_format.bytesPerFrame();
	    nsamples = readx(buffer, nsamples * nblocks) / nblocks;
	    addSamplesRead(nsamples);
	}
	return nsamples;
    }
    void skipSamples(int64_t count)
    {
	int64_t bytes = count * m_format.bytesPerFrame();
	if (seek_forwardx(bytes) != bytes)
	    throw std::runtime_error("seek_forward failed");
    }
private:
    size_t readx(void *buffer, size_t count)
    {
	if (m_ignore_length)
	    return stream().read(buffer, count);
	else
	    return read(buffer, count);
    }
    int64_t seek_forwardx(int64_t count)
    {
	if (m_ignore_length)
	    return stream().seek_forward(count);
	else
	    return seek_forward(count);
    }
    void fetchWaveFormat();
    void parseInfo();
};

#endif
