#ifndef CoreAudioHelper_H
#define CoreAudioHelper_H

#include <cstddef>
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

inline std::wstring CF2W(CFStringRef str)
{
    CFIndex length = CFStringGetLength(str);
    if (!length) return L"";
    std::vector<UniChar> buffer(length);
    CFRange range = { 0, length };
    CFStringGetCharacters(str, range, &buffer[0]);
    return std::wstring(buffer.begin(), buffer.end());
}

typedef x::shared_ptr<const __CFString> CFStringPtr;

inline CFStringPtr W2CF(std::wstring s)
{
    CFStringRef sref = CFStringCreateWithCharacters(0,
	    reinterpret_cast<const UniChar*>(s.c_str()), s.size());
    return CFStringPtr(sref, CFRelease);
}

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

    static size_t calcSize(int n)
    {
	return offsetof(AudioChannelLayout, mChannelDescriptions[1])
		+ std::max(0, n - 1) * sizeof(AudioChannelDescription);
    }
    void attach(const AudioChannelLayout *layout)
    {
	size_t size = calcSize(layout->mNumberChannelDescriptions);
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
	return FromBitmap(chanmap::GetDefaultChannelMask(nchannels));
    }
    static AudioChannelLayoutX FromBitmap(uint32_t bitmap)
    {
	AudioChannelLayoutX layout;
	layout->mChannelLayoutTag = chanmap::GetLayoutTag(bitmap);
	if (layout->mChannelLayoutTag
		== kAudioChannelLayoutTag_UseChannelBitmap)
	    layout->mChannelBitmap = bitmap;
	return layout;
    }
#ifndef REFALAC
    static
    AudioChannelLayoutX FromChannels(const std::vector<uint32_t> &channels)
    {
	AudioChannelLayoutX layout(channels.size());
	layout->mChannelLayoutTag
	    = kAudioChannelLayoutTag_UseChannelDescriptions;
	for (size_t i = 0; i < channels.size(); ++i)
	    layout->mChannelDescriptions[i].mChannelLabel = channels[i];
	UInt32 tag;
	UInt32 size = sizeof tag;
	try {
	    CHECKCA(AudioFormatGetProperty(
		    kAudioFormatProperty_TagForChannelLayout,
		    sizeof(AudioChannelLayout), layout,
		    &size, &tag));
	    layout->mChannelLayoutTag = tag;
	} catch (...) {}
	return layout;
    }
    std::wstring layoutName() const
    {
	CFStringRef name;
	UInt32 size = sizeof(CFStringRef);
	CHECKCA(AudioFormatGetProperty(
		    kAudioFormatProperty_ChannelLayoutName,
		    sizeof(AudioChannelLayout), m_instance.get(),
		    &size, &name));
	std::wstring wname = CF2W(name);
	CFRelease(name);
	return wname;
    }
#endif
private:
    void create(size_t n)
    {
	size_t size = calcSize(n);
	owner_t p = owner_t(
		reinterpret_cast<AudioChannelLayout*>(xcalloc(1, size)),
		std::free);
	p->mNumberChannelDescriptions = n;
	m_instance.swap(p);
    }
};

#endif
