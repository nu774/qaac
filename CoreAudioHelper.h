#ifndef CoreAudioHelper_H
#define CoreAudioHelper_H

#include <vector>
#include <sstream>
#include <stdexcept>
#include "shared_ptr.h"
#include "CoreAudioToolbox.h"
#include "util.h"
#include "chanmap.h"

#define CHECKCA(expr) \
    do { \
	long err = expr; \
	if (err) { \
	    std::stringstream ss; \
	    ss << "ERROR " << err << ": " << #expr; \
	    throw std::runtime_error(ss.str()); \
	} \
    } while (0)

class AudioChannelLayoutX {
    typedef x::shared_ptr<AudioChannelLayout> owner_t;
    owner_t m_instance;
public:
    AudioChannelLayoutX() { create(0); }
    explicit AudioChannelLayoutX(size_t channel_count)
    {
	create(channel_count);
    }
    AudioChannelLayoutX(const AudioChannelLayout &layout)
    {
	attach(&layout);
    }
    AudioChannelLayoutX(const AudioChannelLayout *layout)
    {
	attach(layout);
    }

    operator AudioChannelLayout *() { return m_instance.get(); }
    operator const AudioChannelLayout *() const { return m_instance.get(); }
    AudioChannelLayout *operator->() { return m_instance.get(); }

    void attach(const AudioChannelLayout *layout)
    {
	size_t size =
	    offsetof(AudioChannelLayout,
	    mChannelDescriptions[layout->mNumberChannelDescriptions]);
	owner_t p = owner_t(
	    reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
	    std::free);
	std::memcpy(p.get(), layout, size);
	m_instance.swap(p);
    }
    unsigned numChannels() const
    {
	switch (m_instance->mChannelLayoutTag) {
	case kAudioChannelLayoutTag_UseChannelDescriptions:
	    return m_instance->mNumberChannelDescriptions;
	case kAudioChannelLayoutTag_UseChannelBitmap:
	    return bitcount(m_instance->mChannelBitmap);
	}
	return AudioChannelLayoutTag_GetNumberOfChannels(
		m_instance->mChannelLayoutTag);
    }

    static AudioChannelLayoutX CreateDefault(unsigned nchannels)
    {
	return FromBitmap(GetDefaultChannelMask(nchannels));
    }
    static AudioChannelLayoutX FromChannelMap(const std::vector<uint32_t> &map)
    {
	return FromBitmap(GetChannelMask(map));
    }
    static AudioChannelLayoutX FromBitmap(uint32_t bitmap)
    {
	size_t nc = bitcount(bitmap);
	AudioChannelLayoutX layout(nc);
	layout->mChannelLayoutTag = GetLayoutTag(bitmap);
	if (layout->mChannelLayoutTag
		== kAudioChannelLayoutTag_UseChannelBitmap)
	    layout->mChannelBitmap = bitmap;
	MapChannelLabel(&layout->mChannelDescriptions[0], bitmap);
	return layout;
    }
private:
    void create(size_t channel_count)
    {
	size_t size =
	    offsetof(AudioChannelLayout, mChannelDescriptions[channel_count]);
	owner_t p = owner_t(
		reinterpret_cast<AudioChannelLayout*>(xcalloc(1, size)),
		std::free);
	p->mNumberChannelDescriptions = channel_count;
	m_instance.swap(p);
    }
};

#endif