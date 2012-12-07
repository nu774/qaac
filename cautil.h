#ifndef CAUTIL_H
#define CAUTIL_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include "CoreAudio/CoreFoundation.h"
#include "CoreAudio/CoreAudioTypes.h"
#include "util.h"

#define FOURCC(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

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
            std::stringstream ss; \
            ss << err << ": " << #expr; \
            throw CoreAudioException(msg, err); \
        } \
    } while (0)

namespace cautil {
    std::string make_coreaudio_error(long code, const char *s);

    inline std::wstring CF2W(CFStringRef str)
    {
        CFIndex length = CFStringGetLength(str);
        if (!length) return L"";
        std::vector<UniChar> buffer(length);
        CFRange range = { 0, length };
        CFStringGetCharacters(str, range, &buffer[0]);
        return std::wstring(buffer.begin(), buffer.end());
    }
    inline CFStringPtr W2CF(const std::wstring &s)
    {
        CFStringRef sref = CFStringCreateWithCharacters(0,
                reinterpret_cast<const UniChar*>(s.c_str()), s.size());
        return CFStringPtr(sref, CFRelease);
    }

    CFMutableDictionaryRef CreateDictionary(CFIndex capacity);

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
    AudioStreamBasicDescription
        buildASBDForPCM(double sample_rate, unsigned channels,
                        unsigned bits, unsigned type_flags,
                        unsigned alignment=0);

    AudioStreamBasicDescription
        buildASBDForPCM2(double sample_rate, unsigned channels,
                         unsigned valid_bits, unsigned pack_bits,
                         unsigned type_flags,
                         unsigned alignment=kAudioFormatFlagIsAlignedHigh);
}

#endif
