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

class CoreAudioException: public std::runtime_error
{
    long m_error_code;
public:
    CoreAudioException(const std::string &s, long code)
        : std::runtime_error(s)
    {
        m_error_code = code;
    }
    long code() const { return m_error_code; }
};

#define CHECKCA(expr) \
    do { \
        long err = expr; \
        if (err) { \
            std::string msg = make_coreaudio_error(err, #expr); \
            std::stringstream ss; \
            ss << err << ": " << #expr; \
            throw CoreAudioException(msg, err); \
        } \
    } while (0)

inline std::string make_coreaudio_error(long code, const char *s)
{
    std::stringstream ss;
    int shift;
    for (shift = 0; shift < 32; shift += 8)
        if (!isprint((code >> shift) & 0xff))
            break;
    if (shift == 32)
        ss << s << ": "
           << static_cast<char>(code >> 24)
           << static_cast<char>((code >> 16) & 0xff)
           << static_cast<char>((code >> 8) & 0xff)
           << static_cast<char>(code & 0xff);
    else
        ss << s << ": " << code;
    return ss.str();
}

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

inline
void BuildASBDFromSampleFormat(const SampleFormat &format,
                               AudioStreamBasicDescription *result)
{
    AudioStreamBasicDescription desc = { 0 };
    desc.mFormatID = 'lpcm';
    desc.mFormatFlags =
        (format.m_bitsPerSample & 7) ? kAudioFormatFlagIsAlignedHigh
                                     : kAudioFormatFlagIsPacked;
    if (format.m_type == SampleFormat::kIsSignedInteger)
        desc.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
    else if (format.m_type == SampleFormat::kIsFloat)
        desc.mFormatFlags |= kAudioFormatFlagIsFloat;
    if (format.m_endian == SampleFormat::kIsBigEndian)
        desc.mFormatFlags |= kAudioFormatFlagIsBigEndian;
    desc.mFramesPerPacket = 1;
    desc.mChannelsPerFrame = format.m_nchannels;
    desc.mSampleRate = format.m_rate;
    desc.mBitsPerChannel = format.m_bitsPerSample;
    desc.mBytesPerPacket = desc.mBytesPerFrame = format.bytesPerFrame();
    std::memcpy(result, &desc, sizeof desc);
}

#endif
