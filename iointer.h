#ifndef _IOINTER_H
#define _IOINTER_H

#include <vector>
#include <map>
#include "util.h"

struct SampleFormat {
    enum { kIsSignedInteger, kIsUnsignedInteger, kIsFloat };
    enum { kIsLittleEndian, kIsBigEndian };
    unsigned m_type, m_endian, m_bitsPerSample, m_nchannels, m_rate;

    SampleFormat() :
	m_type(0), m_bitsPerSample(16), m_endian(0), m_nchannels(0), m_rate(0)
    {}
    SampleFormat(const char *spec, unsigned nchannels, unsigned rate);
    uint32_t bytesPerFrame() const
    {
	return m_nchannels * m_bitsPerSample >> 3;
    }
    bool operator==(const SampleFormat &rhs) const
    {
	return m_type == rhs.m_type
	    && m_endian == rhs.m_endian
	    && m_bitsPerSample == rhs.m_bitsPerSample
	    && m_nchannels == rhs.m_nchannels
	    && m_rate == rhs.m_rate;
    }
    bool operator!=(const SampleFormat &rhs) const
    {
	return !operator==(rhs);
    }
    std::string str() const
    {
	static const char *tab[] = { "LE", "BE" };
	return format("%c%d%s", "SUF"[m_type], m_bitsPerSample, tab[m_endian]);
    }
};

struct GaplessInfo {
    uint32_t delay;
    uint32_t padding;
    uint64_t samples;
};

class ISource {
public:
    virtual ~ISource() {}
    virtual uint64_t length() const = 0;
    virtual const SampleFormat &getSampleFormat() const = 0;
    virtual const std::vector<uint32_t> *getChannelMap() const = 0;
    virtual size_t readSamples(void *buffer, size_t nsamples) = 0;
    virtual void setRange(int64_t start=0, int64_t length=-1) = 0;
};

class ISink {
public:
    virtual ~ISink() {}
    virtual void writeSamples(
	    const void *data, size_t len, size_t nsamples) = 0;
};

class ITagParser {
public:
    virtual ~ITagParser() {}
    virtual const std::map<uint32_t, std::wstring> &getTags() const = 0;
    virtual const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const = 0;
};
#endif

