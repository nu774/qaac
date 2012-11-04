#ifndef _IOINTER_H
#define _IOINTER_H

#include <vector>
#include <map>
#include "CoreAudio/CoreAudioTypes.h"
#include "util.h"
#include "chapters.h"

struct GaplessInfo {
    uint32_t delay;
    uint32_t padding;
    uint64_t samples;
};

class ISource {
public:
    virtual ~ISource() {}
    virtual uint64_t length() const = 0;
    virtual const AudioStreamBasicDescription &getSampleFormat() const = 0;
    virtual const std::vector<uint32_t> *getChannels() const = 0;
    virtual size_t readSamples(void *buffer, size_t nsamples) = 0;
    virtual uint64_t getSamplesRead() const = 0;
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
    virtual const std::vector<chapters::entry_t> *getChapters() const = 0;
};

struct IPartialSource {
    virtual ~IPartialSource() {}
    virtual void setRange(int64_t start=0, int64_t length=-1) = 0;
};

template <class Source>
class PartialSource: public ISource, public IPartialSource {
    uint64_t m_duration;
    uint64_t m_samples_read;
public:
    PartialSource(): m_duration(-1), m_samples_read(0) {}
    virtual ~PartialSource() {}
    uint64_t getDuration() const { return m_duration; }
    void addSamplesRead(uint64_t n) { m_samples_read += n; }
    uint64_t getSamplesRead() const { return m_samples_read; }
    uint64_t getRemainingSamples() const {
	return m_duration - m_samples_read;
    }
    size_t adjustSamplesToRead(size_t n) const {
	return static_cast<size_t>(
		std::min(static_cast<uint64_t>(n), getRemainingSamples()));
    }
    void setRange(int64_t start=0, int64_t length=-1) {
	int64_t dur = static_cast<int64_t>(m_duration);
	if (length >= 0 && (dur == -1 || length < dur))
	    m_duration = length;
	if (start)
	    static_cast<Source*>(this)->skipSamples(start);
	if (start > 0 && dur > 0 && length == -1)
	    m_duration -= start;
    }
};

class DelegatingSource: public ISource, public ITagParser {
    std::shared_ptr<ISource> m_src;
    std::map<uint32_t, std::wstring> m_emptyTags;
public:
    DelegatingSource() {}
    DelegatingSource(std::shared_ptr<ISource> src): m_src(src) {}
    void setSource(std::shared_ptr<ISource> src) { m_src = src; }
    ISource *source() { return m_src.get(); }
    uint64_t length() const { return m_src->length(); }
    uint64_t getSamplesRead() const { return m_src->getSamplesRead(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_src->getSampleFormat();
    }
    const std::vector<uint32_t> *getChannels() const
    {
	return m_src->getChannels();
    }
    const std::map<uint32_t, std::wstring> &getTags() const
    {
	ITagParser *parser = dynamic_cast<ITagParser*>(m_src.get());
	if (!parser)
	    return m_emptyTags;
	else
	    return parser->getTags();
    }
    const std::vector<chapters::entry_t> * getChapters() const
    {
	ITagParser *parser = dynamic_cast<ITagParser*>(m_src.get());
	if (!parser)
	    return 0;
	else
	    return parser->getChapters();
    }
    size_t readSamples(void *buffer, size_t nsamples)
    {
	return m_src->readSamples(buffer, nsamples);
    }
};

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *rawBuffer,
			  std::vector<float> *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *byteBuffer,
			  float *floatBuffer, size_t nsamples);
#endif

