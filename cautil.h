#ifndef CAUTIL_H
#define CAUTIL_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <utf8.h>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioToolbox.h>
#else
#include "CoreAudio/CoreFoundation.h"
#include "CoreAudio/CoreAudioTypes.h"
#include "CoreAudio/AudioFile.h"
#endif
#include "util.h"
#include "catypes.h"

typedef std::shared_ptr<const __CFString> CFStringPtr;
typedef std::shared_ptr<const __CFDictionary> CFDictionaryPtr;

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
    bool isNotSupportedError() const
    {
        return m_error_code == FOURCC('t','y','p','?') ||
               m_error_code == FOURCC('f','m','t','?') ||
               m_error_code == FOURCC('p','t','y','?') ||
               m_error_code == FOURCC('c','h','k','?');
    }
};

#define CHECKCA(expr) \
    do { \
        long err = expr; \
        if (err) { \
            std::string msg = cautil::make_coreaudio_error(err, #expr); \
            throw CoreAudioException(msg, err); \
        } \
    } while (0)

namespace cautil {
    std::string make_coreaudio_error(long code, const char *s);

    static_assert(sizeof(ca::AudioStreamBasicDescription) ==
                  sizeof(AudioStreamBasicDescription),
                  "ca::AudioStreamBasicDescription layout mismatch");
    static_assert(sizeof(ca::AudioFilePacketTableInfo) ==
                  sizeof(AudioFilePacketTableInfo),
                  "ca::AudioFilePacketTableInfo layout mismatch");
    static_assert(sizeof(ca::AudioStreamPacketDescription) ==
                  sizeof(AudioStreamPacketDescription),
                  "ca::AudioStreamPacketDescription layout mismatch");

    inline AudioStreamBasicDescription
    toNative(const ca::AudioStreamBasicDescription &x)
    {
        AudioStreamBasicDescription y;
        y.mSampleRate = x.mSampleRate;
        y.mFormatID = x.mFormatID;
        y.mFormatFlags = x.mFormatFlags;
        y.mBytesPerPacket = x.mBytesPerPacket;
        y.mFramesPerPacket = x.mFramesPerPacket;
        y.mBytesPerFrame = x.mBytesPerFrame;
        y.mChannelsPerFrame = x.mChannelsPerFrame;
        y.mBitsPerChannel = x.mBitsPerChannel;
        y.mReserved = x.mReserved;
        return y;
    }
    inline ca::AudioStreamBasicDescription
    fromNative(const AudioStreamBasicDescription &x)
    {
        ca::AudioStreamBasicDescription y;
        y.mSampleRate = x.mSampleRate;
        y.mFormatID = x.mFormatID;
        y.mFormatFlags = x.mFormatFlags;
        y.mBytesPerPacket = x.mBytesPerPacket;
        y.mFramesPerPacket = x.mFramesPerPacket;
        y.mBytesPerFrame = x.mBytesPerFrame;
        y.mChannelsPerFrame = x.mChannelsPerFrame;
        y.mBitsPerChannel = x.mBitsPerChannel;
        y.mReserved = x.mReserved;
        return y;
    }
    inline AudioFilePacketTableInfo
    toNative(const ca::AudioFilePacketTableInfo &x)
    {
        AudioFilePacketTableInfo y;
        y.mNumberValidFrames = x.mNumberValidFrames;
        y.mPrimingFrames = x.mPrimingFrames;
        y.mRemainderFrames = x.mRemainderFrames;
        return y;
    }
    inline ca::AudioFilePacketTableInfo
    fromNative(const AudioFilePacketTableInfo &x)
    {
        ca::AudioFilePacketTableInfo y;
        y.mNumberValidFrames = x.mNumberValidFrames;
        y.mPrimingFrames = x.mPrimingFrames;
        y.mRemainderFrames = x.mRemainderFrames;
        return y;
    }

#ifdef QAAC
    std::string CF2US(CFStringRef str);

    CFStringPtr US2CF(const std::string &s);
#endif

    inline size_t sizeofAudioChannelLayout(const AudioChannelLayout &acl)
    {
        int n = acl.mNumberChannelDescriptions;
        return offsetof(AudioChannelLayout, mChannelDescriptions[1])
                + std::max(0, n - 1) * sizeof(AudioChannelDescription);
    }
    inline int numChannelsOfAudioChannelLayout(const AudioChannelLayout *acl)
    {
        switch (acl->mChannelLayoutTag) {
        case kAudioChannelLayoutTag_UseChannelDescriptions:
            return acl->mNumberChannelDescriptions;
        case kAudioChannelLayoutTag_UseChannelBitmap:
            return util::bitcount(acl->mChannelBitmap);
        }
        return acl->mChannelLayoutTag & 0xffff;
    }
}

#endif
